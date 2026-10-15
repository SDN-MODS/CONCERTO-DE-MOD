class SDN_RaidManagerManager
{
	private static ref SDN_RaidManagerManager s_Instance;

	private const string PROFILE_DIR = "$profile:SDN_MODS/SDN_RaidManager";
	private const string CONFIG_PATH = "$profile:SDN_MODS/SDN_RaidManager/SDN_RaidManagerConfig.json";
	private const string LEGACY_CONFIG_PATH = "$profile:SDN_MODS/SDN_RaidManagerConfig.json";
	private const string LOG_PATH = "$profile:SDN_MODS/SDN_Logs/SDN_RaidManager_Logs/SDN_RaidManager.log";

	private ref SDN_RaidManagerConfig m_Config;
	private bool m_Initialized;
	private ref Timer m_AutoReloadTimer;
	private int m_LastRaidState = -1;
	private int m_LastHeartbeatLogMs = 0;

	private string m_LastConfigContent = "";

	private bool m_ClientRaidActive = false; // Cliente guarda estado aqui

	void SetClientRaidActive(bool state)
	{
		m_ClientRaidActive = state;
	}

	bool GetClientRaidActive()
	{
		return m_ClientRaidActive;
	}

	// O servidor decide pelo horario/configuracao; o cliente usa somente o RPC.
	// Sem estado valido, a destruicao permanece bloqueada.
	bool CanDestroyRaidLock()
	{
		if (!GetGame())
		{
			return false;
		}

		if (GetGame().IsServer())
		{
			return IsSDN_RaidManager();
		}

		return GetClientRaidActive();
	}

	void SyncStateToAll()
	{
		if (!GetGame() || !GetGame().IsServer())
		{
			return;
		}

		bool raidNow = IsSDN_RaidManager();
		
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man player : players)
		{
			if (player && player.GetIdentity())
			{
				ScriptRPC rpc = new ScriptRPC();
				rpc.Write(raidNow);
				rpc.Send(player, 1899911, true, player.GetIdentity());
			}
		}
	}

	void SyncStateToPlayer(Man player)
	{
		if (!GetGame() || !GetGame().IsServer())
		{
			return;
		}

		if (player && player.GetIdentity())
		{
			bool raidNow = IsSDN_RaidManager();
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(raidNow);
			rpc.Send(player, 1899911, true, player.GetIdentity());
		}
	}
	private string m_LastWebhookUrl = "";

	private RestContext m_DiscordContext;
	private string m_DiscordRequestPath = "";

	private ref array<Object> m_AllBases = new array<Object>();

	void RegisterBase(Object baseObj)
	{
		if (baseObj && m_AllBases.Find(baseObj) == -1)
		{
			m_AllBases.Insert(baseObj);
		}
	}

	void UnregisterBase(Object baseObj)
	{
		if (baseObj)
		{
			m_AllBases.RemoveItem(baseObj);
		}
	}

	private void UpdateAllBasesDamageState(bool allowDamage)
	{
		for (int i = m_AllBases.Count() - 1; i >= 0; i--)
		{
			Object b = m_AllBases.Get(i);
			if (!b)
			{
				m_AllBases.RemoveOrdered(i);
				continue;
			}
			b.SetAllowDamage(allowDamage);
		}
	}

	static SDN_RaidManagerManager GetInstance()
	{
		if (!s_Instance)
		{
			s_Instance = new SDN_RaidManagerManager();
		}

		return s_Instance;
	}

	private ref map<string, int> m_RaidBlockedMsgCooldown = new map<string, int>();

	void Init()
	{
		if (m_Initialized)
		{
			return;
		}

		m_Initialized = true;
		EnsureProfileFolder();
		LoadConfig(false);
		StartAutoReload();
		UpdateRaidStateAndNotify();
	}

	bool IsEnabled()
	{
		return m_Config != null;
	}

	string GetRaidUnavailableMessage()
	{
		if (!m_Config)
		{
			return "";
		}

		return m_Config.RaidUnavailablePlayerMessage;
	}

	int GetPlayerMessageCooldownSeconds()
	{
		if (!m_Config)
		{
			return 10;
		}

		return m_Config.PlayerMessageCooldownSeconds;
	}

	bool CanNotifyPlayer(string playerKey)
	{
		int cooldownMs = GetPlayerMessageCooldownSeconds() * 1000;
		int nowMs = GetGame().GetTime();

		int nextAllowedMs = 0;
		if (m_RaidBlockedMsgCooldown.Find(playerKey, nextAllowedMs))
		{
			if (nowMs < nextAllowedMs)
			{
				return false;
			}
		}

		m_RaidBlockedMsgCooldown.Set(playerKey, nowMs + cooldownMs);
		return true;
	}


	bool IsSDN_RaidManager()
	{
		if (!m_Initialized || !m_Config || !m_Config.RaidDays || m_Config.RaidDays.Count() == 0)
		{
			return false;
		}

		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);

		int dayOfWeek = GetDayOfWeek(year, month, day);
		int currentMinuteOfWeek = dayOfWeek * 1440 + hour * 60 + minute;
		const int minutesInWeek = 7 * 1440;

		foreach (RaidDayWindow window : m_Config.RaidDays)
		{
			if (!window)
			{
				continue;
			}

			int startDayIndex = ParseDayOfWeek(window.DayOfWeek);
			if (startDayIndex < 0)
			{
				continue;
			}

			int duration = window.Duration;
			if (duration <= 0)
			{
				continue;
			}

			if (duration >= minutesInWeek)
			{
				return true;
			}

			int startMinute = startDayIndex * 1440 + window.StartHour * 60 + window.StartMinute;
			int endMinute = startMinute + duration;

			if (endMinute < minutesInWeek)
			{
				if (currentMinuteOfWeek >= startMinute && currentMinuteOfWeek < endMinute)
				{
					return true;
				}
			}
			else
			{
				int wrappedEnd = endMinute - minutesInWeek;
				if (currentMinuteOfWeek >= startMinute || currentMinuteOfWeek < wrappedEnd)
				{
					return true;
				}
			}
		}

		return false;
	}

	private void LoadConfig(bool quiet)
	{
		MigrateLegacyConfigIfNeeded();

		if (!FileExist(CONFIG_PATH))
		{
			m_Config = new SDN_RaidManagerConfig();
			NormalizeConfig();
			SaveConfig();
			SetupDiscordContextIfChanged();
			LogInfo("Config created at " + CONFIG_PATH);
			return;
		}

		string currentContent = ReadWholeFile(CONFIG_PATH);
		if (currentContent == "")
		{
			return;
		}

		if (m_LastConfigContent == currentContent)
		{
			return;
		}

		SDN_RaidManagerConfig loadedConfig = new SDN_RaidManagerConfig();
		string errorMessage = "";
		bool loadedOk = JsonFileLoader<SDN_RaidManagerConfig>.LoadFile(CONFIG_PATH, loadedConfig, errorMessage);
		if (!loadedOk)
		{
			LogInfo("Config parse error. Keep previous config. Error: " + errorMessage);
			return;
		}
		m_Config = loadedConfig;
		NormalizeConfig();
		m_LastConfigContent = currentContent;
		SetupDiscordContextIfChanged();

		if (!quiet)
		{
			LogInfo("Config loaded. Windows count: " + m_Config.RaidDays.Count());
		}
	}

	private void SaveConfig()
	{
		if (!m_Config)
		{
			return;
		}

		JsonFileLoader<SDN_RaidManagerConfig>.JsonSaveFile(CONFIG_PATH, m_Config);
		m_LastConfigContent = ReadWholeFile(CONFIG_PATH);
	}

	private void NormalizeConfig()
	{
		if (!m_Config)
		{
			m_Config = new SDN_RaidManagerConfig();
		}

		if (!m_Config.RaidDays)
		{
			m_Config.RaidDays = new array<ref RaidDayWindow>();
		}

		foreach (RaidDayWindow window : m_Config.RaidDays)
		{
			if (!window)
			{
				continue;
			}

			if (window.StartHour < 0) window.StartHour = 0;
			if (window.StartHour > 23) window.StartHour = 23;
			if (window.StartMinute < 0) window.StartMinute = 0;
			if (window.StartMinute > 59) window.StartMinute = 59;
			if (window.Duration < 0) window.Duration = 0;
		}

		if (m_Config.AutoReloadSeconds < 5) m_Config.AutoReloadSeconds = 5;
		if (m_Config.StateLogIntervalSeconds < 30) m_Config.StateLogIntervalSeconds = 30;
		if (m_Config.PlayerMessageCooldownSeconds < 1) m_Config.PlayerMessageCooldownSeconds = 1;
	}

	private void StartAutoReload()
	{
		if (!m_AutoReloadTimer)
		{
			m_AutoReloadTimer = new Timer(CALL_CATEGORY_SYSTEM);
		}

		m_AutoReloadTimer.Stop();
		m_AutoReloadTimer.Run(m_Config.AutoReloadSeconds, this, "OnAutoReloadTick", NULL, true);
		LogInfo("Auto reload started. Interval: " + m_Config.AutoReloadSeconds + " sec");
	}

	void OnAutoReloadTick()
	{
		LoadConfig(true);
		UpdateRaidStateAndNotify();
		// Reenvio periodico cobre clientes cujo objeto ainda nao existia no RPC de entrada.
		SyncStateToAll();
		LogHeartbeatIfNeeded();
	}

	private void UpdateRaidStateAndNotify()
	{
		bool raidNow = IsSDN_RaidManager();
		int state = 0;
		if (raidNow)
		{
			state = 1;
		}

		if (m_LastRaidState == -1)
		{
			m_LastRaidState = state;
			if (raidNow)
			{
				LogInfo("Initial state: RAID ON");
				SendDiscordEvent(m_Config.DiscordStartTitle, m_Config.DiscordStartMessage, 5763719);
			}
			else
			{
				LogInfo("Initial state: RAID OFF");
			}
			UpdateAllBasesDamageState(raidNow);
			return;
		}

		if (m_LastRaidState == state)
		{
			// Reaplicar mesmo sem transicao: construcao/mods podem reativar o dano.
			UpdateAllBasesDamageState(raidNow);
			return;
		}

		m_LastRaidState = state;
		if (raidNow)
		{
			LogInfo("State changed: RAID START");
			SendDiscordEvent(m_Config.DiscordStartTitle, m_Config.DiscordStartMessage, 5763719);
		}
		else
		{
			LogInfo("State changed: RAID END");
			SendDiscordEvent(m_Config.DiscordEndTitle, m_Config.DiscordEndMessage, 15548997);
		}

		UpdateAllBasesDamageState(raidNow);
		SyncStateToAll();
	}

	private void LogHeartbeatIfNeeded()
	{
		if (!m_Config)
		{
			return;
		}

		int nowMs = GetGame().GetTime();
		if ((nowMs - m_LastHeartbeatLogMs) < (m_Config.StateLogIntervalSeconds * 1000))
		{
			return;
		}

		m_LastHeartbeatLogMs = nowMs;
		if (IsSDN_RaidManager())
		{
			LogInfo("Heartbeat: RAID ON | Now: " + GetTimestamp());
		}
		else
		{
			LogInfo("Heartbeat: RAID OFF | Now: " + GetTimestamp());
		}
	}

	private void SetupDiscordContextIfChanged()
	{
		m_DiscordContext = null;
		m_DiscordRequestPath = "";

		if (!m_Config)
		{
			return;
		}

		string webhookUrl = m_Config.DiscordWebhookUrl;
		if (webhookUrl == "")
		{
			m_LastWebhookUrl = "";
			return;
		}

		if (webhookUrl == m_LastWebhookUrl)
		{
			return;
		}

		int protocolIndex = webhookUrl.IndexOf("://");
		if (protocolIndex < 0)
		{
			LogInfo("Discord webhook ignored: invalid URL.");
			return;
		}

		int pathStart = -1;
		for (int i = protocolIndex + 3; i < webhookUrl.Length(); i++)
		{
			if (webhookUrl.Substring(i, 1) == "/")
			{
				pathStart = i;
				break;
			}
		}

		string baseUrl = webhookUrl;
		string requestPath = "";
		if (pathStart >= 0)
		{
			baseUrl = webhookUrl.Substring(0, pathStart);
			requestPath = webhookUrl.Substring(pathStart, webhookUrl.Length() - pathStart);
		}

		RestApi restApi = GetRestApi();
		if (!restApi)
		{
			restApi = CreateRestApi();
		}

		if (!restApi)
		{
			LogInfo("Discord webhook disabled: RestApi unavailable.");
			return;
		}

		m_DiscordContext = restApi.GetRestContext(baseUrl);
		m_DiscordRequestPath = requestPath;
		m_LastWebhookUrl = webhookUrl;

		if (m_DiscordContext)
		{
			m_DiscordContext.SetHeader("application/json");
			LogInfo("Discord webhook configured.");
		}
	}

	private void SendDiscordEvent(string title, string text, int color)
	{
		if (!m_DiscordContext)
		{
			return;
		}

		string payload = "{\"content\":\"[SDN_RaidManager] " + title + " - " + text + "\"}";
		m_DiscordContext.POST_now(m_DiscordRequestPath, payload);
	}

	private string ReadWholeFile(string path)
	{
		FileHandle fh = OpenFile(path, FileMode.READ);
		if (fh == 0)
		{
			return "";
		}

		string line;
		string outText = "";
		while (FGets(fh, line) >= 0)
		{
			outText = outText + line;
		}

		CloseFile(fh);
		return outText;
	}

	private void EnsureProfileFolder()
	{
		MakeDirectory("$profile:SDN_MODS");
		MakeDirectory("$profile:SDN_MODS/SDN_RaidManager");
		MakeDirectory("$profile:SDN_MODS/SDN_Logs");
		MakeDirectory("$profile:SDN_MODS/SDN_Logs/SDN_RaidManager_Logs");
	}

	private void MigrateLegacyConfigIfNeeded()
	{
		if (FileExist(CONFIG_PATH) || !FileExist(LEGACY_CONFIG_PATH))
		{
			return;
		}

		SDN_RaidManagerConfig legacyConfig = new SDN_RaidManagerConfig();
		JsonFileLoader<SDN_RaidManagerConfig>.JsonLoadFile(LEGACY_CONFIG_PATH, legacyConfig);
		m_Config = legacyConfig;
		NormalizeConfig();
		SaveConfig();
		LogInfo("Legacy config migrated to " + CONFIG_PATH);
	}

	private void LogInfo(string message)
	{
		string stamp = GetTimestamp();
		string line = "[SDN_RaidManager] " + stamp + " | " + message;
		Print(line);

		if (!m_Config || !m_Config.EnableFileLog)
		{
			return;
		}

		FileHandle fh = OpenFile(LOG_PATH, FileMode.APPEND);
		if (fh != 0)
		{
			FPrintln(fh, line);
			CloseFile(fh);
		}
	}

	private string GetTimestamp()
	{
		int y;
		int m;
		int d;
		int hh;
		int mm;
		int ss;
		GetYearMonthDay(y, m, d);
		GetHourMinuteSecond(hh, mm, ss);
		return y.ToString() + "-" + Pad2(m) + "-" + Pad2(d) + " " + Pad2(hh) + ":" + Pad2(mm) + ":" + Pad2(ss);
	}

	private string Pad2(int value)
	{
		if (value < 10)
		{
			return "0" + value.ToString();
		}

		return value.ToString();
	}

	private int ParseDayOfWeek(string value)
	{
		string lowered = value;
		lowered.ToLower();

		switch (lowered)
		{
			case "sunday": return 0;
			case "monday": return 1;
			case "tuesday": return 2;
			case "wednesday": return 3;
			case "thursday": return 4;
			case "friday": return 5;
			case "saturday": return 6;
		}

		return -1;
	}

	private int GetDayOfWeek(int year, int month, int day)
	{
		if (month < 1 || month > 12)
		{
			return 0;
		}

		int offset = 0;
		switch (month)
		{
			case 1:  offset = 0; break;
			case 2:  offset = 3; break;
			case 3:  offset = 2; break;
			case 4:  offset = 5; break;
			case 5:  offset = 0; break;
			case 6:  offset = 3; break;
			case 7:  offset = 5; break;
			case 8:  offset = 1; break;
			case 9:  offset = 4; break;
			case 10: offset = 6; break;
			case 11: offset = 2; break;
			case 12: offset = 4; break;
		}

		if (month < 3)
		{
			year = year - 1;
		}

		return (year + year / 4 - year / 100 + year / 400 + offset + day) % 7;
	}
}
