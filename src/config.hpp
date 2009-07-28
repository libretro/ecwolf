// =============================================================================
// ### ### ##   ## ###  #   ###  ##   #   #  ##   ## ### ##  ### ###  #  ###
// #    #  # # # # #  # #   #    # # # # # # # # # # #   # #  #   #  # # #  #
// ###  #  #  #  # ###  #   ##   # # # # # # #  #  # ##  # #  #   #  # # ###
//   #  #  #     # #    #   #    # # # # # # #     # #   # #  #   #  # # #  #
// ### ### #     # #    ### ###  ##   #   #  #     # ### ##  ###  #   #  #  #
//                                     --= http://bitowl.com/sde/ =--
// =============================================================================
// Copyright (C) 2008 "Blzut3" (admin@maniacsvault.net)
// The SDE Logo is a trademark of GhostlyDeath (ghostlydeath@gmail.com)
// =============================================================================
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
// =============================================================================
// Description:
// =============================================================================

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <string>
#include <map>

struct SettingsData
{
	public:
		enum SettingType
		{
			ST_INT,
			ST_STR
		};

		SettingsData(int integer=0) : integer(0), str(""), type(ST_INT) { SetValue(integer); }
		SettingsData(std::string str) : integer(0), str(""), type(ST_STR) { SetValue(str); }

		const int			GetInteger() { return integer; }
		const std::string	GetString()	{ return str; }
		const SettingType	GetType() { return type; }
		void				SetValue(int integer) { this->integer = integer;this->type = ST_INT; }
		void				SetValue(std::string str) { this->str = str;this->type = ST_STR; }

	protected:
		SettingType		type;
		int				integer;
		std::string		str;
};

class Config
{  
	public:
		Config();
		~Config();

		/**
		 * Creates the specified setting if it hasn't been made already.  It 
		 * will be set to the default value.
		 */
		void			CreateSetting(const std::string index, unsigned int defaultInt);
		void			CreateSetting(const std::string index, std::string defaultString);
		/**
		 * Gets the specified setting.  Will return NULL if the setting does 
		 * not exist.
		 */
		SettingsData	*GetSetting(const std::string index);
		/**
		 * Returns if this is an entirely new configuration file.  This can be 
		 * used to see if a first time set up wizard should be run.
		 */
		bool			IsNewConfig() { return firstRun; }
		/**
		 * Looks for the config file and creates the directory if needed.  This 
		 * will call ReadConfig if there is a file to be read.
		 * @see SaveConfig
		 * @see ReadConfig
		 */
		void			LocateConfigFile(int argc, char* argv[]);
		/**
		 * Reads the configuration file for settings.  This is ~/.sde/sde.cfg 
		 * on unix systems.
		 * @see LocateConfigFile
		 * @see SaveConfig
		 */
		void			ReadConfig();
		/**
		 * Saves the configuration to a file.  ~/.sde/sde.cfg on unix systems.
		 * @see LocateConfigFile
		 * @see ReadConfig
		 */
		void			SaveConfig();

		/**
		 * Converts str into a form that can be stored into config files.
		 */
		static const std::string	&Escape(std::string &str);
		static const std::string	&Unescape(std::string &str);
	protected:
		bool			FindIndex(const std::string index, SettingsData *&data);

		bool									firstRun;
		std::string								configFile;
		std::map<std::string, SettingsData *>	settings;
};

extern Config *config;

#endif /* __CONFIG_HPP__ */
