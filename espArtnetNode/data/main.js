var cl = 0;
var num = 0;
var err = 0;
var o = document.getElementsByName('sections');
var s = document.getElementsByName('save');
for (var i = 0, e; e = s[i++];) e.addEventListener('click', function() {
	sendData();
});
var u = document.getElementById('fUp');
var um = document.getElementById('uploadMsg');
var fileSelect = document.getElementById('update');
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
	u.disabled = !0;
	u.value = 'Preparing Device…';
	var x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		if (x.readyState == XMLHttpRequest.DONE) {
			try {
				var r = JSON.parse(x.response)
			} catch (e) {
				var r = {
					success: 0,
					doUpdate: 1
				}
			}
			if (r.success == 1 && r.doUpdate == 1) {
				uploadWait()
			} else {
				um.value = '<b>Update failed!</b>';
				u.value = 'Upload Now';
				u.disabled = !1
			}
		}
	};
	x.open('POST', '/ajax', !0);
	x.setRequestHeader('Content-Type', 'application/json');
	x.send('{\"doUpdate\":1,\"success\":1}')
}

function uploadWait() {
	setTimeout(function() {
		var z = new XMLHttpRequest();
		z.onreadystatechange = function() {
			if (z.readyState == XMLHttpRequest.DONE) {
				try {
					var r = JSON.parse(z.response)
				} catch (e) {
					var r = {
						success: 0
					}
				}
				if (r.success == 1) {
					upload()
				} else {
					uploadWait()
				}
			}
		};
		z.open('POST', '/ajax', !0);
		z.setRequestHeader('Content-Type', 'application/json');
		z.send('{\"doUpdate\":2,\"success\":1}')
	}, 1000)
}

var upload = function() {
	u.value = 'Uploading… 0%';
	var data = new FormData();
	data.append('update', fileSelect.files[0]);
	var x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		if (x.readyState == 4) {
			try {
				var r = JSON.parse(x.response)
			} catch (e) {
				var r = {
					success: 0,
					message: 'No response from device.'
				}
			}
			if (r.success == 1) {
				u.value = r.message;
				setTimeout(function() {
					location.reload()
				}, 15000)
			} else {
				um.value = '<b>Update failed!</b> ' + r.message;
				u.value = 'Upload Now';
				u.disabled = !1
			}
		}
	};
	x.upload.addEventListener('progress', function(e) {
		var p = Math.ceil((e.loaded / e.total) * 100);
		console.log('Progress: ' + p + '%');
		if (p < 100) u.value = 'Uploading... ' + p + '%';
		else u.value = 'Upload complete. Processing…'
	}, !1);
	x.open('POST', '/upload', !0);
	x.send(data)
};

function reboot() {
	if (err == 1) return;
	var r = confirm('Are you sure you want to reboot?');
	if (r != true) return;
	o[cl].className = 'hide';
	o[0].childNodes[0].innerHTML = 'Rebooting';
	o[0].childNodes[1].innerHTML = 'Please wait while the device reboots. This page will refresh shortly unless you changed the IP or Wifi.';
	o[0].className = 'show';
	err = 0;
	var x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		if (x.readyState == 4) {
			try {
				var r = JSON.parse(x.response);
			} catch (e) {
				var r = {
					success: 0,
					message: 'Unknown error: [' + x.responseText + ']'
				};
			}
			if (r.success != 1) {
				o[0].childNodes[0].innerHTML = 'Reboot Failed';
				o[0].childNodes[1].innerHTML = 'Something went wrong and the device didn\'t respond correctly. Please try again.';
			}
			setTimeout(function() {
				location.reload();
			}, 5000);
		}
	};
	x.open('POST', '/ajax', true);
	x.setRequestHeader('Content-Type', 'application/json');
	x.send('{\"reboot\":1,\"success\":1}');
}

function sendData() {
	var d = {
		'page': num
	};
	for (var i = 0, e; e = o[cl].getElementsByTagName('INPUT')[i++];) {
		var k = e.getAttribute('name');
		var v = e.value;
		if (k in d) continue;
		if (k == 'ipAddress' || k == 'subAddress' || k == 'gwAddress' || k == 'portAuni' || k == 'portBuni' || k == 'portAsACNuni' || k == 'portBsACNuni' || k == 'dmxInBroadcast') {
			var c = [v];
			for (var z = 1; z < 4; z++) {
				c.push(o[cl].getElementsByTagName('INPUT')[i++].value);
			}
			d[k] = c;
		} else if (e.type === 'number') {
			if (v == '') v = 0;
			d[k] = v;
		} else if (e.type === 'checkbox') {
			if (e.checked) d[k] = 1;
			else d[k] = 0;
		} else
			d[k] = v;
	}
	for (var i = 0, e; e = o[cl].getElementsByTagName('SELECT')[i++];) {
		d[e.getAttribute('name')] = e.options[e.selectedIndex].value;
	}
	d['success'] = 1;
	var x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		handleAJAX(x);
	};
	x.open('POST', '/ajax');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send(JSON.stringify(d));
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
	var x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		handleAJAX(x);
	};
	x.open('POST', '/ajax');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send(JSON.stringify({"page":num,"success":1}));
}

function loadDebugLog() {
	// page 8 'save' button click goes to standard ajax handler
	const section = document.getElementById('section_debug');
	num = 8;
	o[cl].className = 'hide';
	section.className = 'show';
	cl = 8;

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

function isVisible(e) {
	const style = window.getComputedStyle(e);
	return (style.display !== 'none');
}

function handleAJAX(x) {
	if (x.readyState == XMLHttpRequest.DONE) {
		if (x.status == 200) {
			var response = JSON.parse(x.responseText);
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
				let old_text;
				let button_element;
				for (var i = 0, e; e = s[i++];) {
					if (isVisible(e)) {
						old_text = e.value;
						button_element = e;
						e.value = response['message'];
						e.className = 'showMessage';
					}
				}
				setTimeout(function() {
					button_element.value = old_text;
					button_element.className = '';
				}, 5000);
			}
			o[cl].className = 'hide';
			o[num].className = 'show';
			cl = num;
			for (var key in response) {
				if (response.hasOwnProperty(key)) {
					var a = document.getElementsByName(key);
					if (key == 'ipAddress' || key == 'subAddress') {
						var b = document.getElementsByName(key + 'T');
						for (var z = 0; z < 4; z++) {
							a[z].value = response[key][z];
							if (z == 0) b[0].innerHTML = '';
							else b[0].innerHTML = b[0].innerHTML + ' . ';
							b[0].innerHTML = b[0].innerHTML + response[key][z];
						}
						continue;
					} else if (key == 'bcAddress') {
						for (var z = 0; z < 4; z++) {
							if (z == 0) a[0].innerHTML = '';
							else a[0].innerHTML = a[0].innerHTML + ' . ';
							a[0].innerHTML = a[0].innerHTML + response[key][z];
						}
						continue;
					} else if (key == 'gwAddress' || key == 'dmxInBroadcast' || key == 'portAuni' || key == 'portBuni' || key == 'portAsACNuni' || key == 'portBsACNuni') {
						for (var z = 0; z < 4; z++) {
							a[z].value = response[key][z];
						}
						continue
					}
					if (key == 'portAmode') {
						var b = document.getElementsByName('portApix');
						var c = document.getElementsByName('DmxInBcAddrA');
						if (response[key] >= 30) { // LED mode
							b[0].style.display = '';
							b[1].style.display = '';
						} else {
							b[0].style.display = 'none';
							b[1].style.display = 'none';
						}
						if (response[key] == 2) { // DMX In
							c[0].style.display = '';
						} else {
							c[0].style.display = 'none';
						}
					} else if (key == 'portBmode') {
						var b = document.getElementsByName('portBpix');
						if (response[key] >= 30) { // LED mode
							b[0].style.display = '';
							b[1].style.display = '';
						} else {
							b[0].style.display = 'none';
							b[1].style.display = 'none';
						}
					} else if (key.startsWith("port") && key.endsWith("Status")) { // portAStatus portBStatus, same as mode, decode from number unless string
						const status_num = Number(response[key]);
						let status_str;
						if (status_num === NaN) {
							status_str = response[key];
						} else {
							status_str = mode_map[status_num];
						}
						for (let z = 0; z < a.length; z++) {
							a[z].innerHTML = status_str;
						}
					} else {
						for (var z = 0; z < a.length; z++) {
							switch (a[z].nodeName) {
								case 'P':
								case 'DIV':
									a[z].innerHTML = response[key];
									break;
								case 'INPUT':
									if (a[z].type == 'checkbox') {
										if (response[key] == 1) a[z].checked = true;
										else a[z].checked = false;
									} else a[z].value = response[key];
									break;
								case 'SELECT':
									for (var y = 0; y < a[z].options.length; y++) {
										if (a[z].options[y].value == response[key]) {
											a[z].options.selectedIndex = y;
											break;
										}
									}
									break;
							}
						}
					}
				}
			}
		} else {
			err = 1;
			o[cl].className = 'hide';
			document.getElementsByName('error')[0].className = 'show';
		}
	}
}
var update = document.getElementById('update');
var label = update.nextElementSibling;
var labelVal = label.innerHTML;
update.addEventListener('change', function(e) {
	var fileName = e.target.value.split('\\\\').pop();
	if (fileName) label.querySelector('span').innerHTML = fileName;
	else label.innerHTML = labelVal;
	update.blur();
});
document.onkeydown = function(e) {
	if (cl < 2 || cl > 6) return;
	var e = e || window.event;
	if (e.keyCode == 13) sendData();
};
menuClick(1);