// TODO: complete rewrite into class(es)

let cl = 0;
let num = 0;
let err = 0;
const o = document.getElementsByName('sections');
const s = document.getElementsByName('save');
for (const elem of s) elem.addEventListener('click', function(event) {
	sendData(event.target);
});
const u = document.getElementById('fUp');
const um = document.getElementById('uploadMsg');
const fileSelect = document.getElementById('update');
let settings = {};
u.addEventListener('click', function() {
	uploadPrep()
});

// Add new modes for different RGB ordering
const raw_modes = {
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

let mode_map = {};
add_modes();

function add_modes() {
	const select_A = document.getElementById('portAmode');
	const select_B = document.getElementById('portBmode');
	let option;
	// clear existing
	while (select_A.options.length > 0) {
		select.remove(0);
	}
	while (select_B.options.length > 0) {
		select.remove(0);
	}

	for (const key in raw_modes) {
		if (key.startsWith("na_")) continue;
		option = document.createElement("option");
		option.text = key;
		option.value = raw_modes[key];
		select_A.add(option);
		select_B.add(option.cloneNode(true));
		mode_map[raw_modes[key]] = key;
	}
}

function uploadPrep() {
	if (fileSelect.files.length === 0) return;
	u.disabled = true;
	u.value = 'Preparing Device…';
	const x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		if (x.readyState == XMLHttpRequest.DONE) {
			let response;
			try {
				response = JSON.parse(x.responseText)
			} catch (e) {
				response = {
					success: 0,
					doUpdate: 1
				}
			}
			if (response.success == 1 && response.doUpdate == 1) {
				uploadWait();
			} else {
				um.value = '<b>Update failed!</b>';
				u.value = 'Upload Now';
				u.disabled = false;
			}
		}
	};
	x.open('POST', '/ajax');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send('{"doUpdate":1,"success":1}')
}

function uploadWait() {
	setTimeout(function() {
		const z = new XMLHttpRequest();
		z.onreadystatechange = function() {
			if (z.readyState == XMLHttpRequest.DONE) {
				let response;
				try {
					response = JSON.parse(z.responseText)
				} catch (e) {
					response = {
						success: 0
					}
				}
				if (response.success == 1) {
					upload();
				} else {
					uploadWait();
				}
			}
		};
		z.open('POST', '/ajax');
		z.setRequestHeader('Content-Type', 'application/json');
		z.send('{"doUpdate":2,"success":1}')
	}, 5000)
}

function upload() {
	u.value = 'Uploading… 0%';
	const data = new FormData();
	data.append('update', fileSelect.files[0]);
	const x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		if (x.readyState == XMLHttpRequest.DONE) {
			let response;
			try {
				response = JSON.parse(x.responseText)
			} catch (e) {
				response = {
					success: 0,
					message: 'No response from device.'
				}
			}
			if (response.success == 1) {
				u.value = response.message;
				setTimeout(function() {
					location.reload()
				}, 15000)
			} else {
				um.value = '<b>Update failed!</b> ' + response.message;
				u.value = 'Upload Now';
				u.disabled = false;
			}
		}
	};
	x.upload.addEventListener('progress', function(e) {
		const p = Math.ceil((e.loaded / e.total) * 100);
		if (p < 100) u.value = 'Uploading... ' + p + '%';
		else u.value = 'Upload complete. Processing…'
	}, false);
	x.open('POST', '/upload');
	x.send(data)
};

function reboot() {
	if (err == 1) return;
	const r = confirm('Are you sure you want to reboot?');
	if (!r) return;
	o[cl].className = 'hide';
	o[0].childNodes[0].innerHTML = 'Rebooting';
	o[0].childNodes[1].innerHTML = 'Please wait while the device reboots. This page will refresh shortly unless you changed the IP or Wifi.';
	o[0].className = 'show';
	err = 0;
	const x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		if (x.readyState == XMLHttpRequest.DONE) {
			let response;
			try {
				const response = JSON.parse(x.responseText);
			} catch (e) {
				const response = {
					success: 0,
					message: 'Unknown error: [' + x.responseText + ']'
				};
			}
			if (response.success != 1) {
				o[0].childNodes[0].innerHTML = 'Reboot Failed';
				o[0].childNodes[1].innerHTML = 'Something went wrong and the device didn\'t respond correctly. Please try again.';
			}
			setTimeout(function() {
				location.reload();
			}, 5000);
		}
	};
	x.open('POST', '/ajax');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send('{"reboot":1,"success":1}');
}

function sendData(buttonClicked) {
	const data = {
		'page': num
	};
	let button_found = false;
	for (const elem of o[cl].getElementsByTagName('INPUT')) {
		if (!button_found) {
			if (buttonClicked.isSameNode(elem)) button_found = true;
			continue;
		}
		const key = elem.getAttribute('name');
		let value = elem.value;
		if (Array.isArray(settings[key])) {
			value = Number(value);
			if (key in data) {
				data[key].push(value);
			} else {
				data[key] = [value];
			}
		} else if (elem.type === 'number') {
			data[key] = Number(value);
		} else if (elem.type === 'checkbox') {
			if (elem.checked) data[key] = 1;
			else data[key] = 0;
		} else if (elem.type === 'button') {
			// stop at the next button
			break;
		} else
			data[key] = value;
	}
	// does not cater for multiple buttons
	for (const elem of o[cl].getElementsByTagName('SELECT')) {
		data[elem.getAttribute('name')] = Number(elem.options[elem.selectedIndex].value);
	}
	data['success'] = 1;
	const x = new XMLHttpRequest();
	x.onreadystatechange = () => {
		if (x.readyState == XMLHttpRequest.DONE) {
			loadSettings([x, buttonClicked]);
		}
	};
	x.open('POST', '/ajax');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send(JSON.stringify(data));
}

function menuClick(n) {
	if (err == 1) return;
	num = n;
	setTimeout(function() {
		if (cl == num || err == 1) return;
		o[cl].className = 'hide';
		o[0].className = 'show';
		cl = 0;
	}, 100);
	const x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		handleAJAX(x);
	};
	x.open('POST', '/ajax');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send(JSON.stringify({"page":num,"success":1}));
}

function misc() {
	// page 8 'save' button click goes to standard ajax handler
	const section = document.getElementById('misc');
	num = 8;
	o[cl].className = 'hide';
	section.className = 'show';
	cl = 8;
	loadDebugLog();
}

function loadDebugLog() {
	const x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		if (x.readyState == XMLHttpRequest.DONE && x.status == 200) {
			document.getElementById('debugLog').value = x.responseText;
		}
	};
	x.open('GET', '/debug.log');
	x.setRequestHeader('Content-Type', 'text/plain');
	x.send();
}

// Settings are loaded from settings.json in two scenarios:
// 1. page is changed (menuclick)
// 2. data is saved (button click)
function loadSettings(inp) {
	const x = new XMLHttpRequest();
	x.onreadystatechange = () => {
		if (x.readyState == XMLHttpRequest.DONE) {
			updateUI(x);
			if (typeof inp === "number") menuClick(inp);
			else handleAJAX(...inp); // array [x, buttonClicked]
		}
	};
	x.open('GET', '/settings.json');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send();
}

function updateUI(request) {
	if (request.readyState == XMLHttpRequest.DONE) {
		if (request.status == 200) {
			const response = JSON.parse(request.responseText);
			// response could be a full settings
			for (let key in response) {

				const input = document.getElementsByName(key);
				// most likely an empty list:
				let stat = document.getElementsByName(key + 'T');

				// Special processing
				if (key === "success" || key == "message") {
					// don't save
				} else if (key === "isHotspot") {
					if (response[key]) {
						settings["wifiStatus"] = `Hotspot started, SSID: ${settings['hotspotSSID']}`;
					} else {
						settings["wifiStatus"] = `Wifi connected, SSID: ${settings['wifiSSID']}`;
					}
					key = "wifiStatus";
				} else {
					// merge into settings
					settings[key] = response[key];
				}

				// run on everything
				for (let z = 0; z < input.length; z++) {
					switch (input[z].nodeName) {
						case 'P':
						case 'DIV':
							input[z].innerHTML = settings[key];
							break;
						case 'INPUT':
							if (input[z].type == 'checkbox') input[z].checked = Boolean(settings[key]);
							else input[z].value = settings[key];
							break;
						case 'SELECT':
							for (let y = 0; y < input[z].options.length; y++) {
								if (input[z].options[y].value == settings[key]) {
									input[z].options.selectedIndex = y;
									break;
								}
							}
							break;
					} // switch
				} // for elem

				if (Array.isArray(settings[key])) {
					for (let z = 0; z < settings[key].length; z++) {
						if (input.length) input[z].value = settings[key][z];
						if (stat.length) {
							if (z == 0) stat[0].innerHTML = '';
							else stat[0].innerHTML = stat[0].innerHTML + '.';
							stat[0].innerHTML = stat[0].innerHTML + settings[key][z];
						}
					}
					continue;
				} else if (key.startsWith("port")) {
					const chan = key[4];
					if (key.endsWith("mode")) {
						const pix = document.getElementsByName("port" + chan + "pix");
						const bcaddr = document.getElementsByName("DmxInBcAddr");
						stat = document.getElementsByName("port" + chan + "Status");
						const status_num = Number(settings[key]);
						let status_str;
						if (pix.length && status_num >= 30) { // LED mode
							for (const elem of pix) elem.style.display = '';
						} else {
							for (const elem of pix) elem.style.display = 'none';
						}
						if (bcaddr.length && status_num == 2) { // DMX In
							bcaddr[0].style.display = '';
						} else {
							bcaddr[0].style.display = 'none';
						}
						// portAStatus portBStatus, same as mode, decode from number unless string
						if (status_num === NaN) {
							status_str = settings[key];
						} else {
							status_str = mode_map[status_num];
						}
						if (stat.length) {
							for (let z = 0; z < stat.length; z++) {
								stat[z].innerHTML = status_str;
							}
						}
					}
				} else if (key === "numPorts" && settings[key] === 1) {
					// Remove Port B
					for (const elem of document.getElementsByName("portB")) {
						elem.style.display = 'none';
					}
				}
			} // for key
		} // status == 200
		else { // request status error
			err = 1;
			o[cl].className = 'hide';
			document.getElementsByName('error')[0].className = 'show';
		}
	} // ajax = DONE
}

function isVisible(e) {
	const style = window.getComputedStyle(e);
	return (style.display !== 'none');
}

function handleAJAX(x, triggerElement = null) {
	if (x.readyState == XMLHttpRequest.DONE) {
		if (x.status == 200) {
			const response = JSON.parse(x.responseText);
			if (!response.hasOwnProperty('success')) {
				err = 1;
				o[cl].className = 'hide';
				document.getElementsByName('error')[0].className = 'show';
				return;
			}
			if (response['success'] != 1) {
				err = 1;
				o[cl].className = 'hide';
				document.getElementsByName('error')[0].getElementsByTagName('P')[0].innerHTML = response['message'];
				document.getElementsByName('error')[0].className = 'show';
				return;
			}
			if (response.hasOwnProperty('message')) {
				let button_element = triggerElement;
				let old_text = triggerElement.value;
				if (!button_element) {
					for (const elem of s) {
						if (isVisible(elem)) {
							old_text = elem.value;
							button_element = elem;
						}
					}
				}
				button_element.value = response['message'];
				button_element.className = 'showMessage';
				setTimeout(() => {
					button_element.value = old_text;
					button_element.className = '';
				}, 5000);
			}
			o[cl].className = 'hide';
			o[num].className = 'show';
			cl = num;
			updateUI(x);
		}
	}
}
const update = document.getElementById('update');
const label = update.nextElementSibling;
const labelVal = label.innerHTML;
update.addEventListener('change', function(e) {
	const fileName = e.target.value.split('\\\\').pop();
	if (fileName) label.querySelector('span').innerHTML = fileName;
	else label.innerHTML = labelVal;
	update.blur();
});
document.onkeydown = function(e) {
	if (cl < 2 || cl > 6) return;
	e = e || window.event;
	if (e.keyCode == 13) sendData();
};
loadSettings(1);
