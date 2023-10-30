/*
ESP8266_ArtNetNode v2.1.0
Copyright (c) 2023, Richard Case
https://github.com/casesolved-co-uk/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/

UI Language support
[TODO] implement different languages
[TODO] get default language from browser
[TODO] find & replace default text in the HTML
*/

const languages = {};

languages.en = {
	confirm_reboot: "Are you sure you want to reboot?",
	err_comm: "There was an error communicating with the device. Refresh the page and try again.",
	err_ajax_parse: "There was a problem with the returned data.",
	err_fail: "Your device refused the request. Check the debug log for further information.",
	// msg and title suffixes must match:
	msg_boot: "Fetching data from device. If this message is still here in 15 seconds, try refreshing the page or clicking the menu option again.",
	msg_reboot: "Please wait while the device reboots. This page will refresh shortly unless you changed the IP or Wifi settings.",
	msg_disabled: "Disabled in hotspot mode",
	msg_upload_now: "Upload Now",
	msg_upload_prep: "Preparing Device...",
	msg_uploading: "Uploading...",
	msg_upload_complete: "Upload complete. Processing...",
	title_boot: "Connecting",
	title_reboot: "Rebooting",
	// generic titles:
	title_error: "Error",
	title_message: "Message",
};
