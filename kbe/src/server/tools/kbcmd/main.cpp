/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2017 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "server/kbemain.h"
#include "kbcmd.h"
#include "client_templates.h"
#include "entitydef/entitydef.h"

#undef DEFINE_IN_INTERFACE
#include "machine/machine_interface.h"
#define DEFINE_IN_INTERFACE
#include "machine/machine_interface.h"

#undef DEFINE_IN_INTERFACE
#include "client_lib/client_interface.h"
#define DEFINE_IN_INTERFACE
#include "client_lib/client_interface.h"

#undef DEFINE_IN_INTERFACE
#include "baseappmgr/baseappmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "baseappmgr/baseappmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#include "cellappmgr/cellappmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "cellappmgr/cellappmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#include "cellapp/cellapp_interface.h"
#define DEFINE_IN_INTERFACE
#include "cellapp/cellapp_interface.h"

#undef DEFINE_IN_INTERFACE
#include "baseapp/baseapp_interface.h"
#define DEFINE_IN_INTERFACE
#include "baseapp/baseapp_interface.h"

#undef DEFINE_IN_INTERFACE
#include "loginapp/loginapp_interface.h"
#define DEFINE_IN_INTERFACE
#include "loginapp/loginapp_interface.h"

#undef DEFINE_IN_INTERFACE
#include "dbmgr/dbmgr_interface.h"
#define DEFINE_IN_INTERFACE
#include "dbmgr/dbmgr_interface.h"

#undef DEFINE_IN_INTERFACE
#include "tools/logger/logger_interface.h"
#define DEFINE_IN_INTERFACE
#include "tools/logger/logger_interface.h"

#undef DEFINE_IN_INTERFACE
#include "tools/bots/bots_interface.h"
#define DEFINE_IN_INTERFACE
#include "tools/bots/bots_interface.h"

#undef DEFINE_IN_INTERFACE
#include "tools/interfaces/interfaces_interface.h"
#define DEFINE_IN_INTERFACE
#include "tools/interfaces/interfaces_interface.h"

using namespace KBEngine;

#define PARSE_COMMAND_ARG_BEGIN()	\
	for (int argIdx = 1; argIdx < argc; ++argIdx)	\
	{	\
		std::string cmd = argv[argIdx];	\
		std::string findcmd;	\
		std::string::size_type fi1;	\

#define PARSE_COMMAND_ARG_DO_FUNC(NAME, EXEC)	\
	cmd = argv[argIdx];	\
	findcmd = NAME;	\
	fi1 = cmd.find(findcmd); \
	if (fi1 != std::string::npos)	\
	{	\
		cmd.erase(fi1, findcmd.size());	\
		try \
		{ \
			if (EXEC == -1) \
				return -1; \
		} \
		catch (...) \
		{ \
			ERROR_MSG("parseCommandArgs: "#NAME"? invalid, no set! type is uint64\n"); \
		} \
		\
		continue; \
	} \

#define PARSE_COMMAND_ARG_GET_VALUE(NAME, VAL)	\
	cmd = argv[argIdx];	\
	findcmd = NAME;	\
	fi1 = cmd.find(findcmd);	\
	if (fi1 != std::string::npos)	\
	{	\
		cmd.erase(fi1, findcmd.size());	\
		if (cmd.size() > 0)	\
		{	\
			try \
			{ \
				VAL = cmd;	\
			} \
			catch (...) \
			{ \
				ERROR_MSG("parseCommandArgs: "#NAME"? invalid, no set! type is uint64\n"); \
			} \
		} \
		 \
		continue; \
	} \

#define PARSE_COMMAND_ARG_END()	}

int process_make_client_templates(int argc, char* argv[], const std::string clientType)
{
	Resmgr::getSingleton().initialize();
	setEvns();

	DebugHelper::initialize(g_componentType);

	INFO_MSG("-----------------------------------------------------------------------------------------\n\n\n");

	Resmgr::getSingleton().print();

	Network::EventDispatcher dispatcher;
	DebugHelper::getSingleton().pDispatcher(&dispatcher);

	Network::g_SOMAXCONN = g_kbeSrvConfig.tcp_SOMAXCONN(g_componentType);

	Network::NetworkInterface networkInterface(&dispatcher);

	DebugHelper::getSingleton().pNetworkInterface(&networkInterface);

	KBCMD app(dispatcher, networkInterface, g_componentType, g_componentID);

	START_MSG(COMPONENT_NAME_EX(g_componentType), g_componentID);

	if (!app.initialize())
	{
		ERROR_MSG("app::initialize(): initialization failed!\n");

		app.finalise();

		// 如果还有日志未同步完成， 这里会继续同步完成才结束
		DebugHelper::getSingleton().finalise();
		return -1;
	}

	std::vector<PyTypeObject*>	scriptBaseTypes;
	if (!EntityDef::initialize(scriptBaseTypes, g_componentType)) 
	{
		ERROR_MSG("app::initialize(): EntityDef initialization failed!\n");

		app.finalise();

		// 如果还有日志未同步完成， 这里会继续同步完成才结束
		DebugHelper::getSingleton().finalise();
		return -1;
	}

	std::string path = "";

	PARSE_COMMAND_ARG_BEGIN();
	PARSE_COMMAND_ARG_GET_VALUE("--outpath=", path);
	PARSE_COMMAND_ARG_END();

	ClientTemplates* pTemplates = ClientTemplates::createClientTemplates(clientType);
	
	int ret = 0;

	if (pTemplates)
	{
		try
		{
			if (!pTemplates->create(path))
			{
				ret = -1;
			}
		}
		catch (std::exception &err)
		{
			ERROR_MSG(fmt::format("app::initialize(): create templates error({})!\n", err.what()));
		}
	}
	else
	{
		ERROR_MSG(fmt::format("app::initialize(): create templates error! nonsupport type={}\n", clientType));
		ret = -1;
	}

	app.finalise();
	INFO_MSG(fmt::format("{}({}) has shut down. ClientTemplates={}\n", COMPONENT_NAME_EX(g_componentType), g_componentID, pTemplates->good()));

	// 如果还有日志未同步完成， 这里会继续同步完成才结束
	DebugHelper::getSingleton().finalise();

	if(pTemplates)
		delete pTemplates;

	return ret;
}

int process_help(int argc, char* argv[])
{
	printf("Usage:\n");
	printf("--clienttemplates:\n");
	printf("\tAutomatically generate client code based on entity_defs file. Environment variables based on KBE.\n");
	printf("\tkbcmd.exe --clienttemplates=unity --outpath=c:/unity_clienttemplates\n");
	printf("\tkbcmd.exe --clienttemplates=ue4 --outpath=c:/unity_clienttemplates\n");
	printf("\tkbcmd.exe --clienttemplates=ue4 --outpath=c:/unity_clienttemplates --KBE_ROOT=\"*\"  --KBE_RES_PATH=\"*\"  --KBE_BIN_PATH=\"*\"\n");

	printf("\n--help:\n");
	printf("\tDisplay help information.\n");
	return 0;
}

int main(int argc, char* argv[])
{
	g_componentType = TOOL_TYPE;
	g_componentID = 0;

	if (argc == 1)
	{
		return process_help(argc, argv);
	}

	parseMainCommandArgs(argc, argv);

	PARSE_COMMAND_ARG_BEGIN();
	PARSE_COMMAND_ARG_DO_FUNC("--clienttemplates=", process_make_client_templates(argc, argv, cmd));
	PARSE_COMMAND_ARG_DO_FUNC("--help", process_help(argc, argv));
	PARSE_COMMAND_ARG_END();

	return 0;
}

