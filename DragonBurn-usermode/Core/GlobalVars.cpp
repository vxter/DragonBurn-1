#include "globalvars.h"
#include "../Helpers/Logger.h"
#include <sstream>

namespace
{
	std::string ToHex(DWORD64 value)
	{
		std::ostringstream oss;
		oss << "0x" << std::uppercase << std::hex << value;
		return oss.str();
	}
}

bool globalvars::UpdateGlobalvars()
{
	DWORD64 m_DglobalVars = 0;
	DWORD64 globalVarsPtr = gGame.GetGlobalVarsAddress();
	if (!memoryManager.ReadMemory<DWORD64>(globalVarsPtr, m_DglobalVars))
	{
		Log::Warning("GlobalVars pointer read failed at " + ToHex(globalVarsPtr) +
			" (client.dll " + ToHex(gGame.GetClientDLLAddress()) +
			" + Offset.GlobalVars " + ToHex(Offset.GlobalVars) + ")");
		return false;
	}
	if (m_DglobalVars == 0)
	{
		Log::Warning("GlobalVars pointer is null at " + ToHex(globalVarsPtr) +
			" (client.dll " + ToHex(gGame.GetClientDLLAddress()) +
			" + Offset.GlobalVars " + ToHex(Offset.GlobalVars) + "), skipping GlobalVars initialization");
		return false;
	}

	this->address = m_DglobalVars;

	auto logFieldFail = [this](const char* field, DWORD offset)
	{
		Log::Warning(std::string("GlobalVars ") + field + " read failed at " +
			ToHex(this->address + offset) + " (base " + ToHex(this->address) +
			" + offset " + ToHex(offset) + ")");
	};

	if (!this->GetRealTime())
	{
		logFieldFail("RealTime", Offset.GlobalVar.RealTime);
		return false;
	}
	if (!this->GetFrameCount())
	{
		logFieldFail("FrameCount", Offset.GlobalVar.FrameCount);
		return false;
	}
	if (!this->GetMaxClients())
	{
		logFieldFail("MaxClients", Offset.GlobalVar.MaxClients);
		return false;
	}
	if (!this->GetTickCount())
	{
		logFieldFail("TickCount", Offset.GlobalVar.TickCount);
		return false;
	}
	if (!this->GetIntervalPerTick())
	{
		logFieldFail("IntervalPerTick", Offset.GlobalVar.IntervalPerTick);
		return false;
	}
	if (!this->GetIntervalPerTick2())
	{
		logFieldFail("IntervalPerTick2", Offset.GlobalVar.IntervalPerTick2);
		return false;
	}
	if (!this->GetcurrentTime())
	{
		logFieldFail("CurrentTime", Offset.GlobalVar.CurrentTime);
		return false;
	}
	if (!this->GetcurrentTime2())
	{
		logFieldFail("CurrentTime2", Offset.GlobalVar.CurrentTime2);
		return false;
	}
	if (!this->GetCurrentNetchan())
	{
		logFieldFail("CurrentNetchan", Offset.GlobalVar.CurrentNetchan);
		return false;
	}
	if (!this->GetCurrentMap())
	{
		logFieldFail("CurrentMap", Offset.GlobalVar.CurrentMap);
		return false;
	}
	if (!this->GetCurrentMapName())
	{
		logFieldFail("CurrentMapName", Offset.GlobalVar.CurrentMapName);
		return false;
	}

	return true;
}

bool globalvars::GetRealTime()
{
	return GetDataAddressWithOffset<float>(this->address, Offset.GlobalVar.RealTime, this->g_fRealTime);
}

bool globalvars::GetFrameCount()
{
	return GetDataAddressWithOffset<int>(this->address, Offset.GlobalVar.FrameCount, this->g_iFrameCount);
}

bool globalvars::GetMaxClients()
{
	return GetDataAddressWithOffset<int>(this->address, Offset.GlobalVar.MaxClients, this->g_iMaxClients);
}

bool globalvars::GetTickCount()
{
	return GetDataAddressWithOffset<int>(this->address, Offset.GlobalVar.TickCount, this->g_iTickCount);
}

bool globalvars::GetIntervalPerTick()
{
	return GetDataAddressWithOffset<float>(this->address, Offset.GlobalVar.IntervalPerTick, this->g_fIntervalPerTick);
}

bool globalvars::GetIntervalPerTick2()
{
	return GetDataAddressWithOffset<float>(this->address, Offset.GlobalVar.IntervalPerTick2, this->g_fIntervalPerTick2);
}

bool globalvars::GetcurrentTime()
{
	return GetDataAddressWithOffset<float>(this->address, Offset.GlobalVar.CurrentTime, this->g_fCurrentTime);
}

bool globalvars::GetcurrentTime2()
{
	return GetDataAddressWithOffset<float>(this->address, Offset.GlobalVar.CurrentTime2, this->g_fCurrentTime2);
}

bool globalvars::GetCurrentNetchan()
{
	return GetDataAddressWithOffset<void*>(this->address, Offset.GlobalVar.CurrentNetchan, this->g_vCurrentNetchan);
}

bool globalvars::GetCurrentMap()
{
	return GetDataAddressWithOffset<char*>(this->address, Offset.GlobalVar.CurrentMap, this->g_cCurrentMap);
}

bool globalvars::GetCurrentMapName()
{
	return GetDataAddressWithOffset<char*>(this->address, Offset.GlobalVar.CurrentMapName, this->g_cCurrentMapName);
}
