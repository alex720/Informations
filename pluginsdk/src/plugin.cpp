/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2016 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include <string>
#include <thread>

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 23
#define PLUGIN_VERSION "1"

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 7000
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"Too Much Informations";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "Too Much Informations";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "Too Much Informations";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return PLUGIN_VERSION;
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "shitty720";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Too Much Informations";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    printf("client user data: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE);

	//printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("client user data: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	//printf("PLUGIN: offersConfigure\n");
	/*
	 * Return values:
	 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	 */
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {
   // printf("PLUGIN: configure\n");
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	//printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return "";
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
	
	return 0;  /* Plugin handled command */
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
    printf("PLUGIN: currentServerConnectionChanged %llu (%llu)\n", (long long unsigned int)serverConnectionHandlerID, (long long unsigned int)ts3Functions.getCurrentServerConnectionHandlerID());
}

/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
	return "Informations";
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */

std::string getChannelCodec(int a) {

	switch (a) {
	case CODEC_SPEEX_NARROWBAND:
		return std::string("CODEC_SPEEX_NARROWBAND");
		break;
	case CODEC_SPEEX_WIDEBAND:
		return std::string("CODEC_SPEEX_WIDEBAND");
		break;
	case CODEC_SPEEX_ULTRAWIDEBAND:
		return std::string("CODEC_SPEEX_ULTRAWIDEBAND");
		break;
	case CODEC_CELT_MONO:
		return std::string("CODEC_CELT_MONO");
		break;
	case CODEC_OPUS_VOICE:
		return std::string("CODEC_OPUS_VOICE");
		break;
	case CODEC_OPUS_MUSIC:
		return std::string("CODEC_OPUS_MUSIC");
		break;
	default:
		return std::string("error");
		break;
	}
}

template <typename T>
std::string convertoString(T a) {
	return std::to_string(a);
}

void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
	std::string infodata = ""; 
	char *buffer = "";
	int bufferInt = 0;
	uint64 bufferUInt = 0;
	double bufferD = 0;

	

	/* For demonstration purpose, display the name of the currently selected server, channel or client. */
	switch (type) {
	case PLUGIN_SERVER: {
		//myid
		anyID myid;
		ts3Functions.getClientID(serverConnectionHandlerID, &myid);


		//server UID
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_UNIQUE_IDENTIFIER, &buffer) != ERROR_ok) {
			printf("Error getting VIRTUALSERVER_UNIQUE_IDENTIFIER\n");

		}
		else {
			infodata += "ServerUID = ";
			infodata += buffer;// copy the VIRTUALSERVER_UNIQUE_IDENTIFIER into infodata
			infodata += "\n";// copy a return into infodata


		}


		//VIRTUALSERVER_ID
		//ts3Functions.requestServerVariables(serverConnectionHandlerID);

		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_ID, &bufferUInt) != ERROR_ok) {
			printf("Error getting VIRTUALSERVER_ID\n");

		}

		else {

			infodata += "Virtualserver ID = ";
			infodata += convertoString<uint64>(bufferUInt);// copy the VIRTUALSERVER_ID into infodata
			infodata += "\n";// copy a return into infodata

		}

		//CONNECTION_INFO
		if (ts3Functions.requestConnectionInfo(serverConnectionHandlerID, myid, NULL) != ERROR_ok) {
			printf("Error getting ConnectionInfo\n");

		}

		if (ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, myid, CONNECTION_SERVER_IP, &buffer) != ERROR_ok) {
			printf("Error getting Connection Server IP\n");
		}

		else {
			infodata += "Serverip = ";
			infodata += buffer; // copy the Serverip into infodata
			infodata += "\n";// copy a return into infodata

		}

		//port
		if (ts3Functions.getServerVariableAsInt(serverConnectionHandlerID, VIRTUALSERVER_PORT, &bufferInt) != ERROR_ok) {
			printf("Error getting Virtualserver Port\n");

		}
		else {
			infodata += "Virtualserver Port = ";
			infodata += convertoString<int>(bufferInt);// copy the VIRTUALSERVER_PORT into infodata
			//infodata += "\n";// copy a return into infodata
		}
		break;
	}
	case PLUGIN_CHANNEL: {
		//channelid
		infodata += "Channel ID = ";
		infodata += convertoString<uint64>(id); // copy the ChannelID into infodata
		infodata += "\n";// copy a return into infodata


		//channelOrderID

		if (ts3Functions.getChannelVariableAsUInt64(serverConnectionHandlerID, id, CHANNEL_ORDER, &bufferUInt) != ERROR_ok) {
			printf("Error getting CHANNEL ORDER ID \n");

		}
		else {
			infodata += "Order ID = ";
			infodata += convertoString<uint64>(bufferUInt);// copy the CHANNEL_ORDER into infodata
			infodata += "\n";// copy a return into infodata

		}

		//pheotischername

		if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, id, CHANNEL_NAME_PHONETIC, &buffer) != ERROR_ok) {
			printf("Error getting CHANNEL_NAME_PHONETIC\n");

		}

		else {
			infodata += "Phoetic Channelname = ";
			infodata += buffer;// copy the CHANNEL_NAME_PHONETIC into infodata
			infodata += "\n";// copy a return into infodata
		}
		/*
		//channelcodec
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_CODEC_QUALITY, &bufferInt) != ERROR_ok) {
			printf("Error getting Channel Code\n");
		}
		else {
			infodata += "Channel Codec = ";
			infodata += getChannelCodec(bufferInt);// copy the Channel Codec into infodata
			infodata += "\n";// copy a return into infodata

		}*/

		//channelcodec qualität
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_CODEC_QUALITY, &bufferInt) != ERROR_ok) {
			printf("Error getting Codec Quality\n");
		}
		else {
			infodata += "Codec Quality = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata

		}

		//CHANNEL_FLAG_PERMANENT
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_PERMANENT, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_PERMANENT\n");
		}
		else {
			infodata += "CHANNEL_FLAG_PERMANENT = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata

		}
		//CHANNEL_FLAG_SEMI_PERMANENT
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_SEMI_PERMANENT, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_SEMI_PERMANENT\n");
		}
		else {
			infodata += "CHANNEL_FLAG_SEMI_PERMANENT = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata

		}
		//CHANNEL_FLAG_DEFAULT
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_DEFAULT, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_DEFAULT\n");
		}
		else {
			infodata += "CHANNEL_FLAG_DEFAULT = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_FLAG_PASSWORD
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_PASSWORD, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_PASSWORD\n");
		}
		else {
			infodata += "CHANNEL_FLAG_PASSWORD = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_CODEC_LATENCY_FACTOR
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_CODEC_LATENCY_FACTOR, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_CODEC_LATENCY_FACTOR\n");
		}
		else {
			infodata += "CHANNEL_CODEC_LATENCY_FACTOR = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_CODEC_IS_UNENCRYPTED
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_CODEC_IS_UNENCRYPTED, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_CODEC_IS_UNENCRYPTED\n");
		}
		else {
			infodata += "CHANNEL_CODEC_IS_UNENCRYPTED = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_DELETE_DELAY
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_DELETE_DELAY, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_DELETE_DELAY\n");
		}
		else {
			infodata += "CHANNEL_DELETE_DELAY = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_FLAG_MAXCLIENTS_UNLIMITED
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_MAXCLIENTS_UNLIMITED, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_MAXCLIENTS_UNLIMITED\n");
		}
		else {
			infodata += "CHANNEL_FLAG_MAXCLIENTS_UNLIMITED = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_FLAG_MAXFAMILYCLIENTS_UNLIMITED
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_MAXFAMILYCLIENTS_UNLIMITED, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_MAXFAMILYCLIENTS_UNLIMITED\n");
		}
		else {
			infodata += "CHANNEL_FLAG_MAXFAMILYCLIENTS_UNLIMITED = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_FLAG_MAXFAMILYCLIENTS_INHERITED
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_MAXFAMILYCLIENTS_INHERITED, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_MAXFAMILYCLIENTS_INHERITED\n");
		}
		else {
			infodata += "CHANNEL_FLAG_MAXFAMILYCLIENTS_INHERITED = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_FLAG_ARE_SUBSCRIBED
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_ARE_SUBSCRIBED, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_ARE_SUBSCRIBED\n");
		}
		else {
			infodata += "CHANNEL_FLAG_ARE_SUBSCRIBED = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_NEEDED_TALK_POWER
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_NEEDED_TALK_POWER, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_NEEDED_TALK_POWER\n");
		}
		else {
			infodata += "CHANNEL_NEEDED_TALK_POWER = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_FORCED_SILENCE
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FORCED_SILENCE, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FORCED_SILENCE\n");
		}
		else {
			infodata += "CHANNEL_FORCED_SILENCE = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_ICON_ID
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_ICON_ID, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_ICON_ID\n");
		}
		else {
			infodata += "CHANNEL_ICON_ID = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CHANNEL_FLAG_PRIVATE
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, id, CHANNEL_FLAG_PRIVATE, &bufferInt) != ERROR_ok) {
			printf("Error getting CHANNEL_FLAG_PRIVATE\n");
		}
		else {
			infodata += "CHANNEL_FLAG_PRIVATE = ";
			infodata += convertoString<int>(bufferInt);// copy the Codec Quality into infodata
			//infodata += "\n";// copy a return into infodata
		}

		break;
	}
	case PLUGIN_CLIENT: {
		//ConnectionInfo

		if (ts3Functions.requestConnectionInfo(serverConnectionHandlerID, (anyID)id, NULL) != ERROR_ok) {
			printf("Error getting ConnectionInfo\n");

		}

		// Client data
		//ClientID

		infodata += "Client ID = ";
		infodata += convertoString<uint64>(id);// copy the ClientID into infodata
		infodata += "\n";// copy a return into infodata

		//CLIENT_UNIQUE_IDENTIFIER
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_UNIQUE_IDENTIFIER, &buffer) != ERROR_ok) {
			printf("Error getting client UID\n");

		}
		else {
			infodata += "UID = ";
			infodata += buffer;// copy the UID into infodata
			infodata += "\n";// copy a return into infodata
		}
		//clientdatabaseID
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_DATABASE_ID, &bufferInt) != ERROR_ok) {
			printf("Error getting client DatabaseID\n");

		}
		else {
			infodata += "DBID = ";
			infodata += convertoString<int>(bufferInt);// copy the DBID into infodata
			infodata += "\n";// copy a return into infodata
		}


		//ClientServergroups
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_SERVERGROUPS, &buffer) != ERROR_ok) {
			printf("Error getting client Servergroups\n");
		}
		else {

			infodata += "ServerGroups = ";
			infodata += buffer;// copy the servergroupids into infodata
			infodata += "\n";// copy a return into infodata

		}


		//totalConnections
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_TOTALCONNECTIONS, &bufferInt) != ERROR_ok) {
			printf("Error getting client TOTALCONNECTIONS\n");

		}
		else {
			infodata += "Total Connections = ";
			infodata += convertoString<int>(bufferInt);// copy the totalconnections into infodata
			infodata += "\n";// copy a return into infodata						
		}


		//Ping
		Sleep(115);
		if (ts3Functions.getConnectionVariableAsDouble(serverConnectionHandlerID, (anyID)id, CONNECTION_PING, &bufferD) != ERROR_ok) {
			printf("Error getting client Ping\n");
		}

		else {


			infodata += "Ping = ";
			infodata += convertoString<int>((int)bufferD); // cast to int to lost .00000   copy the PING into infodata
			infodata += "\n";// copy a return into infodata	
		}


		//pheotischername
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_NICKNAME_PHONETIC, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_NICKNAME_PHONETIC\n");
		
		}
		infodata += "Phonetic Nickname = ";
		infodata += buffer;// copy the Client phonetic Nickname into infodata
		infodata += "\n";// copy a return into infodata	

	//version sign
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_VERSION_SIGN, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_VERSION_SIGN\n");
		
		}
		else {
			infodata += "Client Version Sign = ";
			infodata += buffer;// copy the Client version sign into infodata	
			infodata += "\n";// copy a return into infodata	
		}
	//badgetid
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_BADGES, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_BADGES\n");
		
		}
		else {
			infodata += "Client BadgetIDs = ";
			infodata += buffer;// copy the CLIENT_BADGES into infodata
			infodata += "\n";// copy a return into infodata
		}
		 //CLIENT_FLAG_TALKING
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_FLAG_TALKING, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_FLAG_TALKING\n");
		
		}
		else {
			infodata += "CLIENT_FLAG_TALKING = ";
			infodata += buffer;// copy the CLIENT_FLAG_TALKING into infodata
			infodata += "\n";// copy a return into infodata
		}
		 //CLIENT_INPUT_MUTED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_INPUT_MUTED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_INPUT_MUTED\n");
		
		}
		else {
			infodata += "CLIENT_INPUT_MUTED = ";
			infodata += buffer;// copy the CLIENT_INPUT_MUTED into infodata
			infodata += "\n";// copy a return into infodata	
		}

		//CLIENT_OUTPUT_MUTED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_OUTPUT_MUTED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_OUTPUT_MUTED\n");
		
		}
		else {
			infodata += "CLIENT_OUTPUT_MUTED = ";
			infodata += buffer;// copy the CLIENT_OUTPUT_MUTED into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CLIENT_OUTPUTONLY_MUTED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_OUTPUTONLY_MUTED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_OUTPUTONLY_MUTED\n");
		
		}
		else {
			infodata += "CLIENT_OUTPUTONLY_MUTED = ";
			infodata += buffer;// copy the CLIENT_OUTPUTONLY_MUTED into infodata
			infodata += "\n";// copy a return into infodata
		}
		//CLIENT_INPUT_HARDWARE
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_INPUT_HARDWARE, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_INPUT_HARDWARE\n");
			
		}
		else {
			infodata += "CLIENT_INPUT_HARDWARE = ";
			infodata += buffer;// copy the CLIENT_INPUT_HARDWARE into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_OUTPUT_HARDWARE
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_OUTPUT_HARDWARE, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_OUTPUT_HARDWARE\n");
			
		}
		else {
			infodata += "CLIENT_OUTPUT_HARDWARE = ";
			infodata += buffer;// copy the CLIENT_OUTPUT_HARDWARE into infodata
			infodata += "\n";// copy a return into infodat
		}
		 //CLIENT_IS_RECORDING
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_IS_RECORDING, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_IS_RECORDING\n");
			
		}
		else {
			infodata += "CLIENT_IS_RECORDING = ";
			infodata += buffer;// copy the CLIENT_IS_RECORDING into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_CHANNEL_GROUP_ID
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_CHANNEL_GROUP_ID, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_CHANNEL_GROUP_ID\n");
			
		}
		else {
			infodata += "CLIENT_CHANNEL_GROUP_ID = ";
			infodata += buffer;// copy the CLIENT_CHANNEL_GROUP_ID into infodata
			infodata += "\n";// copy a return into infodat
		}
		
		//CLIENT_CREATED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_CREATED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_CREATED\n");
			
		}
		else {
			infodata += "CLIENT_CREATED = ";
			infodata += buffer;// copy the CLIENT_CREATED into infodata
			infodata += "\n";// copy a return into infodat
		}

		//CLIENT_LASTCONNECTED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_LASTCONNECTED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_LASTCONNECTED\n");

		}
		else {
			infodata += "CLIENT_LASTCONNECTED = ";
			infodata += buffer;// copy the CLIENT_LASTCONNECTED into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_AWAY
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_AWAY, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_AWAY\n");

		}
		else {
			infodata += "CLIENT_AWAY = ";
			infodata += buffer;// copy the CLIENT_AWAY into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_AWAY_MESSAGE
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_AWAY_MESSAGE, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_AWAY_MESSAGE\n");

		}
		else {
			infodata += "CLIENT_AWAY_MESSAGE = ";
			infodata += buffer;// copy the CLIENT_AWAY_MESSAGE into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_TYPE
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_TYPE, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_TYPE\n");

		}
		else {
			infodata += "CLIENT_TYPE = ";
			infodata += buffer;// copy the CLIENT_TYPE into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_FLAG_AVATAR
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_FLAG_AVATAR, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_FLAG_AVATAR\n");

		}
		else {
			infodata += "CLIENT_FLAG_AVATAR = ";
			infodata += buffer;// copy the CLIENT_FLAG_AVATAR into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_TALK_POWER
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_TALK_POWER, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_TALK_POWER\n");

		}
		else {
			infodata += "CLIENT_TALK_POWER = ";
			infodata += buffer;// copy the CLIENT_TALK_POWER into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_IS_TALKER
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_IS_TALKER, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_IS_TALKER\n");

		}
		else {
			infodata += "CLIENT_IS_TALKER = ";
			infodata += buffer;// copy the CLIENT_IS_TALKER into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_MONTH_BYTES_UPLOADED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_MONTH_BYTES_UPLOADED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_MONTH_BYTES_UPLOADED\n");

		}
		else {
			infodata += "CLIENT_MONTH_BYTES_UPLOADED = ";
			infodata += buffer;// copy the CLIENT_MONTH_BYTES_UPLOADED into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_MONTH_BYTES_DOWNLOADED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_MONTH_BYTES_DOWNLOADED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_MONTH_BYTES_DOWNLOADED\n");

		}
		else {
			infodata += "CLIENT_MONTH_BYTES_DOWNLOADED = ";
			infodata += buffer;// copy the CLIENT_MONTH_BYTES_DOWNLOADED into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_TOTAL_BYTES_UPLOADED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_TOTAL_BYTES_UPLOADED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_TOTAL_BYTES_UPLOADED\n");

		}
		else {
			infodata += "CLIENT_TOTAL_BYTES_UPLOADED = ";
			infodata += buffer;// copy the CLIENT_TOTAL_BYTES_UPLOADED into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_TOTAL_BYTES_DOWNLOADED
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_TOTAL_BYTES_DOWNLOADED, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_TOTAL_BYTES_DOWNLOADED\n");

		}
		else {
			infodata += "CLIENT_TOTAL_BYTES_DOWNLOADED = ";
			infodata += buffer;// copy the CLIENT_TOTAL_BYTES_DOWNLOADED into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_IS_PRIORITY_SPEAKER
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_IS_PRIORITY_SPEAKER, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_IS_PRIORITY_SPEAKER\n");

		}
		else {
			infodata += "CLIENT_IS_PRIORITY_SPEAKER = ";
			infodata += buffer;// copy the CLIENT_IS_PRIORITY_SPEAKER into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_UNREAD_MESSAGES
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_UNREAD_MESSAGES, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_UNREAD_MESSAGES\n");

		}
		else {
			infodata += "CLIENT_UNREAD_MESSAGES = ";
			infodata += buffer;// copy the CLIENT_UNREAD_MESSAGES into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_NEEDED_SERVERQUERY_VIEW_POWER
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_NEEDED_SERVERQUERY_VIEW_POWER, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_NEEDED_SERVERQUERY_VIEW_POWER\n");

		}
		else {
			infodata += "CLIENT_NEEDED_SERVERQUERY_VIEW_POWER = ";
			infodata += buffer;// copy the CLIENT_NEEDED_SERVERQUERY_VIEW_POWER into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_ICON_ID
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_ICON_ID, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_ICON_ID\n");

		}
		else {
			infodata += "CLIENT_ICON_ID = ";
			infodata += buffer;// copy the CLIENT_ICON_ID into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_IS_CHANNEL_COMMANDER
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_IS_CHANNEL_COMMANDER, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_IS_CHANNEL_COMMANDER\n");

		}
		else {
			infodata += "CLIENT_IS_CHANNEL_COMMANDER = ";
			infodata += buffer;// copy the CLIENT_IS_CHANNEL_COMMANDER into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_COUNTRY
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_COUNTRY, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_COUNTRY\n");

		}
		else {
			infodata += "CLIENT_COUNTRY = ";
			infodata += buffer;// copy the CLIENT_COUNTRY into infodata
			infodata += "\n";// copy a return into infodat
		}
		//CLIENT_CHANNEL_GROUP_INHERITED_CHANNEL_ID
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_CHANNEL_GROUP_INHERITED_CHANNEL_ID, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_CHANNEL_GROUP_INHERITED_CHANNEL_ID\n");

		}
		else {
			infodata += "CLIENT_CHANNEL_GROUP_INHERITED_CHANNEL_ID = ";
			infodata += buffer;// copy the CLIENT_CHANNEL_GROUP_INHERITED_CHANNEL_ID into infodata
			infodata += "\n";// copy a return into infodat
		}

		//meta data
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_META_DATA, &buffer) != ERROR_ok) {
			printf("Error getting CLIENT_META_DATA\n");
			
		}
		else {
			infodata += "Client metadata = ";
			infodata += buffer; // copy a the CLIENT_CLIENT_META_DATA into infodata
			//infodata += "\n";// copy a return into infodata
		}
		break;
	}
		default:
			printf("Invalid item type: %d\n", type);
			data = NULL;  /* Ignore */
			return;
	}
	/* Must be allocated in the plugin! */
	*data = (char*)malloc((infodata.length() + 1)* sizeof(char));
	snprintf(*data, (infodata.length() + 1), infodata.c_str());
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);



/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	 * Create the menus
	 * There are three types of menu items:
	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	 *
	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	 *
	 * The menu text is required, max length is 128 characters
	 *
	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	 * plugin filename, without dll/so/dylib suffix
	 * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	 */

	BEGIN_CREATE_MENUS(0);  /* IMPORTANT: Number of menu items must be correct! */
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

	/*
	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
	 * If unused, set menuIcon to NULL
	 */
	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

	/*
	 * Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
	 * Test it with plugin command: /test enablemenu <menuID> <0|1>
	 * Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
	 * ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
	 * menu is displayed.
	 */
	/* For example, this would disable MENU_ID_GLOBAL_2: */
	/* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */

	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}

/* Helper function to create a hotkey */
static struct PluginHotkey* createHotkey(const char* keyword, const char* description) {
	struct PluginHotkey* hotkey = (struct PluginHotkey*)malloc(sizeof(struct PluginHotkey));
	_strcpy(hotkey->keyword, PLUGIN_HOTKEY_BUFSZ, keyword);
	_strcpy(hotkey->description, PLUGIN_HOTKEY_BUFSZ, description);
	return hotkey;
}

/* Some makros to make the code to create hotkeys a bit more readable */
#define BEGIN_CREATE_HOTKEYS(x) const size_t sz = x + 1; size_t n = 0; *hotkeys = (struct PluginHotkey**)malloc(sizeof(struct PluginHotkey*) * sz);
#define CREATE_HOTKEY(a, b) (*hotkeys)[n++] = createHotkey(a, b);
#define END_CREATE_HOTKEYS (*hotkeys)[n++] = NULL; assert(n == sz);

/*
 * Initialize plugin hotkeys. If your plugin does not use this feature, this function can be omitted.
 * Hotkeys require ts3plugin_registerPluginID and ts3plugin_freeMemory to be implemented.
 * This function is automatically called by the client after ts3plugin_init.
 */
void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys) {
	/* Register hotkeys giving a keyword and a description.
	 * The keyword will be later passed to ts3plugin_onHotkeyEvent to identify which hotkey was triggered.
	 * The description is shown in the clients hotkey dialog. */
	BEGIN_CREATE_HOTKEYS(0);  /* Create 3 hotkeys. Size must be correct for allocating memory. */
	END_CREATE_HOTKEYS;

	/* The client will call ts3plugin_freeMemory to release all allocated memory */
}

