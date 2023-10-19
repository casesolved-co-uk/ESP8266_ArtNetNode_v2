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

JS Controller
*/

let language = languages.en;


// Does the initial settings load and opens "status" section
class pageHandler {
	// Add new modes for different RGB ordering
	static raw_modes = {
		"DMX Output":0,
		"DMX Output with RDM":1,
		"DMX Input":2,
		"na_WS2811_400":32,
		"WS2811":36,
		"WS2812":40,
		"na_WS2812B":44,
		"WS2813":48,
		"na_NEOPIXEL":52,
		"na_WS2852":56,
		"na_APA104":60,
		"na_APA106":64,
		"na_SM16703":68,
		"na_TM1829":72,
		"na_TM1812":76,
		"na_TM1809":80,
		"na_TM1804":84,
		"na_TM1803":88,
		"na_UCS1903":92,
		"na_UCS1903B":96,
		"na_UCS1904":100,
		"na_UCS2903":104,
		"na_GS1903":108,
		"na_SK6812":112,
		"na_SK6822":116,
		"na_PL9823":120,
		"na_SK6822":124,
		"na_GE8822":128,
		"na_GW6205":132,
		"na_GW6205_400":136,
		"na_LPD1886":140,
		"na_LPD1886_8BIT":144
	}
	constructor() {
		this.refresh();
		this.add_modes();
	}
	refresh() {
		ajax("/settings.json", (err, json) => this.loadSettings(err, json));
	}
	add_modes() {
		this.mode_map = {};
		let option;

		for (const key in pageHandler.raw_modes) {
			if (key.startsWith("na_")) continue;
			this.mode_map[pageHandler.raw_modes[key]] = key;
		}

		const modeElems = [];
		for (const select of document.getElementsByTagName("SELECT"))
			if (select.name.startsWith("port") && select.name.endsWith("mode"))
				modeElems.push(select);
		for (const select of modeElems) {
			// clear existing
			while (select.options.length > 0) {
				select.remove(0);
			}

			for (const num in this.mode_map) {
				option = document.createElement("option");
				option.text = this.mode_map[num];
				option.value = Number(num);
				select.add(option);
			}
		}
	}
	loadSettings(err, json) {
		if (!err) {
			// new page
			if (!this.settings) {
				this.settings = json;
				new sectionHandler("status");
			}
			this.updateSettings(err, json);
			this.updatePage();
		}
	}
	updateSettings(err, json) {
		// also used for button save return values
		// we don't run updatePage here because a full settings reload will be triggered
		// after a button save
		if (!err) {
			for (let key in json) {
				if (key === "success" || key == "message") {
					// don't save
				} else if (key === "isHotspot") {
					if (json[key]) {
						settings["wifiStatus"] = `Hotspot started, SSID: ${this.settings['hotspotSSID']}`;
					} else {
						settings["wifiStatus"] = `Wifi connected, SSID: ${this.settings['wifiSSID']}`;
					}
				} else {
					// merge into settings
					this.settings[key] = json[key];
				}
			}
		}
	}
	updatePage(){
		for (const key in this.settings) {
			const input = document.getElementsByName(key);

			// update everything first
			for (const elem of input) {
				switch (elem.nodeName) {
					case 'P':
					case 'DIV':
						elem.innerHTML = this.settings[key];
						break;
					case 'INPUT':
						if (elem.type === 'checkbox') elem.checked = Boolean(this.settings[key]);
						else elem.value = this.settings[key];
						break;
					case 'SELECT':
						for (let y = 0; y < elem.options.length; y++) {
							if (elem.options[y].value == this.settings[key]) {
								elem.options.selectedIndex = y;
								break;
							}
						}
						break;
				}
			}

			// ip address arrays
			if (Array.isArray(this.settings[key])) {
				// A couple of text ip addresses, e.g. ipAddressT:
				const text = document.getElementsByName(key + 'T');
				for (let z = 0; z < this.settings[key].length; z++) {
					if (input.length) input[z].value = this.settings[key][z];
					if (text.length) {
						if (z == 0) text[0].innerHTML = "";
						else text[0].innerHTML = text[0].innerHTML + '.';
						text[0].innerHTML = text[0].innerHTML + String(this.settings[key][z]);
					}
				}
				continue;
			} else if (key.startsWith("port")) {
				const channel = key[4];
				if (key.endsWith("mode")) {
					const pix = document.getElementsByName("port" + channel + "pix");
					const bcaddr = document.getElementsByName("DmxInBcAddr");
					const status_num = Number(this.settings[key]);
					let status_str;
					// show/hide LED fields
					if (pix.length) {
						if (status_num >= 30) { // LED mode
							for (const elem of pix) elem.style.display = '';
						} else {
							for (const elem of pix) elem.style.display = 'none';
						}
					}
					// show/hide DMXIn fields
					if (bcaddr.length) {
						if (status_num == 2) { // DMX In
							for (const elem of bcaddr) elem.style.display = '';
						} else {
							for (const elem of bcaddr) elem.style.display = 'none';
						}
					}
					// create port status, same as mode, decode from number unless string
					if (status_num === NaN) {
						status_str = this.settings[key];
					} else {
						status_str = this.mode_map[status_num];
					}
					// assign port status
					for (const elem of document.getElementsByName("port" + channel + "Status")) {
						elem.innerHTML = status_str;
					}
				}
			} else if (key === "numPorts" && this.settings[key] === 1) {
				// Remove Port B
				for (const elem of document.getElementsByName("portB")) {
					elem.style.display = 'none';
				}
			}
		} // for key
	}
	closeCurrent(){
		if (this.current)
			this.current.className = "hide";
	}
	open(element){
		this.closeCurrent();
		this.current = element;
		this.current.className = "show";
	}
}

class sectionHandler {
	constructor(elem_id) {
		this.section = document.getElementById(elem_id);
		this.saveBind();
		this.init();
		this.display();
	}
	init() {
		// for subclasses
	}
	display() {
		page.open(this.section);
	}
	saveBind() {
		for (const button of this.section.getElementsByTagName("INPUT")) {
			if (button.type === "button" && button.name === "save")
				button.addEventListener("click", (e) => this.save(e));
		}
	}
	save(event) {
		this.saveButton = event.target;
		const json = this.getData(event.target);
		ajax("/ajax", (e, j) => this.save_result(e, j), json);
	}
	save_result(err, json) {
		if (!err) {
			if (json.message) {
				const old_text = this.saveButton.value;
				this.saveButton.value = json.message;
				this.saveButton.className = "showMessage";
				setTimeout(() => {
					this.saveButton.value = old_text;
					this.saveButton.className = "";
				}, 5000);
			}
			page.updateSettings(err, json);
			setTimeout(page.refresh, 200);
		}
	}
	getData(button) {
		const data = {
			"section": this.section.id
		};
		let button_found = false;
		for (const input of this.section.getElementsByTagName("INPUT")) {
			if (!button_found) {
				if (button.isSameNode(input)) button_found = true;
				continue;
			}
			const key = input.name;
			let value = input.value;
			if (input.type === "number") {
				value = Number(value);
				if (key in data) {
					if (typeof data[key] === "number") data[key] = [data[key]];
					data[key].push(value);
				} else {
					data[key] = value;
				}
			} else if (input.type === "checkbox") {
				if (input.checked) data[key] = 1;
				else data[key] = 0;
			} else if (input.type === "button") {
				// stop at the next button
				break;
			} else
				data[key] = value;
		}
		// does not cater for multiple buttons
		for (const select of this.section.getElementsByTagName("SELECT")) {
			data[select.getAttribute("name")] = Number(select.options[select.selectedIndex].value);
		}
		return data;
	}
}

class portHandler extends sectionHandler {
	init() {
		this.changeBind();
	}
	changeBind() {
		// display LED pixel config
		this.pixElements = this.section.querySelectorAll("span[name='portpix']");
		for (const select of this.section.getElementsByTagName("SELECT")) {
			if (select.name.endsWith("mode")) {
				select.addEventListener("change", (e) => this.modeChange(e));
				this.modeChange({target: {value: select.value}});
				break;
			}
		}
		// calculate artnet universe addresses
		let found;
		this.uni = [];
		for (const input of this.section.getElementsByTagName("INPUT")) {
			found = true;
			if (input.name.endsWith("net")) this.net = input;
			else if (input.name.endsWith("sub")) this.sub = input;
			else if (input.name.endsWith("uni") && !input.name.endsWith("sACNuni")) this.uni.push(input);
			else found = false;
			if (found) input.addEventListener("change", (e) => this.addrChange(e));
		}
		this.addrChange();
	}
	modeChange(event) {
		if (Number(event.target.value) >= 30) { // LED mode
			for (const elem of this.pixElements) elem.style.display = "";
		} else {
			for (const elem of this.pixElements) elem.style.display = "none";
		}
	}
	addrChange(event) {
		let address;
		for (const uni of this.uni) {
			address = Number(this.net.value) << 8 | Number(this.sub.value) << 4 | Number(uni.value);
			uni.nextSibling.innerHTML = "(" + address + ")";
		}
	}
}

// controls display of errors and other messages
class messageHandler extends sectionHandler {
	init() {
		const headings = this.section.getElementsByTagName("H2");
		if (headings.length) this.title = headings[0];
		const paras = this.section.getElementsByTagName("P");
		if (paras.length) this.firstP = paras[0];
	}
	display() {
		// do nothing
	}
	_show(title, msg) {
		this.title.innerHTML = title;
		this.firstP.innerHTML = msg;
		// controls visibility independently of pageHandler
		this.section.className = 'show';
	}
	_hide() {
		this.section.className = 'hide';
	}
	// msg_id may be an id from the language list or a passed messge
	decode(msg_id) {
		let title;
		let msg;
		if (msg_id.startsWith("err_"))
			title = language["title_error"];
		else if (msg_id.startsWith("msg_"))
			title = language["title_" + msg_id.slice(4)];
		else
			title = language["title_message"];
		if (language[msg_id])
			msg = language[msg_id];
		else
			msg = msg_id;
		return [title, msg];
	}
	show(msg_id) {
		const res = this.decode(msg_id);
		this._show(...res);
	}
	showOnly(msg_id) {
		// close other sections first
		page.closeCurrent();
		this.show(msg_id);
	}
	toast(msg_id) {
		this.show(msg_id);
		setTimeout(() => {
			this._hide();
		}, 10000);
	}
}

const message = new messageHandler("message");
const page = new pageHandler();

class rebootHandler {
	constructor() {
		if (window.confirm(language.confirm_reboot))
			this.reboot();
	}
	reboot() {
		message.showOnly("msg_reboot");
		ajax("/ajax", postjs='{"reboot":1}', reload=5000);
	}
}

class firmwareHandler extends sectionHandler {
	init() {
		this.buttonBind();
	}
	buttonBind() {
		this.button = this.section.getElementById("fUp");
		this.msg = this.section.getElementById("uploadMsg");
		this.button.addEventListener("click", this.prep);
		this.section.getElementById("update").addEventListener("change", this.fileSelect);
	}
	fileSelect(event) {
		const label = event.target.nextElementSibling;
		const labelVal = label.innerHTML;
		this.file = event.target.files[0];
		fileName = event.target.value.split("\\\\").pop();
		if (fileName) label.querySelector("span").innerHTML = fileName;
		else label.innerHTML = labelVal;
	}
	prep(event) {
		if (!this.file) return;
		this.button.disabled = true;
		this.button.value = language["msg_upload_prep"];
		ajax("/ajax", (e, j) => this.wait(e, j), '{"doUpdate":1}', msgElem=this.msg);
	}
	wait(err, json) {
		if (err) {
			this.reset();
		} else {
			setTimeout(function() {
				ajax("/ajax", (e, j) => this.upload(e, j), '{"doUpdate":2}', msgElem=this.msg);
			}, 5000);
		}
	}
	upload(err, json) {
		if (err) {
			this.reset();
		} else {
			ajax("/upload", (e, j) => this.reset(e, j), reload=15000, msgElem=this.msg, file=this.file, progressElem=this.button);
		}
	}
	reset() {
		this.button.value = language["msg_upload_now"];
		this.button.disabled = false;
		setTimeout(function() {
			this.msg.innerHTML = "";
		}, 5000);
	}
}

class miscHandler extends sectionHandler {
	init() {
		ajax("/debug.log", (e,t) => this.log_result(e,t), parse=false);
	}
	log_result(err, text) {
		if (!err)
			this.section.getElementById('debugLog').value = text;
	}
}

// callback has the form cb(err, json)
function ajax(url, callback=null, postjs=null, reload=0, parse=true, msgElem=null, file=null, progressElem=null) {
	const request = new XMLHttpRequest();
	let msgFunc;

	if (msgElem) {
		msgFunc = (msg) => {
			const result = message.decode(msg);
			msgElem.innerHTML = result[1];
		}
	} else {
		// already does decode
		msgFunc = (msg) => {
			message.toast(msg);
		}
	}

	if (file) {
		const data = new FormData();
		data.append("update", file);
	}

	request.onreadystatechange = function() {
		if (request.readyState == XMLHttpRequest.DONE) {
			let response;
			let err = 1;
			if (request.status != 200)
				msgFunc("err_comm");
			else {
				if (parse) {
					try {
						response = JSON.parse(request.responseText);
						if (response.success === 1 || response.success === undefined) {
							err = 0;
							if (response.message) msgFunc(response.message);
						} else {
							if (response.message) msgFunc(response.message);
							else msgFunc("err_fail");
						}
					} catch (e) {
						msgFunc("err_ajax_parse");
					}
				} else {
					response = request.responseText;
					err = 0;
				}
			}
			if (callback) {
				callback(err, response);
			}
			if (!err && reload) {
				setTimeout(function() {
					location.reload();
				}, reload);
			}
		}
	};

	if (progressElem) {
		progressElem.value = language["msg_uploading"];
		request.upload.addEventListener("progress", function(e) {
			const p = Math.ceil((e.loaded / e.total) * 100);
			if (p < 100) progressElem.value = language["msg_uploading"] + " " + p + "%";
			else progressElem.value = language["msg_upload_complete"];
		}, false);
	}

	if (file) {
		request.open("POST", url);
		request.send(postjs);
	} else if (postjs) {
		request.open("POST", url);
		request.setRequestHeader("Content-Type", "application/json");
		request.send(postjs);
	} else {
		request.open("GET", url);
		request.send();
	}
}
