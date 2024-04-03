//The content of this file get set into htmlHeadStyle (new_http.cs)

var firstTime,
	lastTime,
	req = null;

var getElement = (id) => document.getElementById(id);

// refresh status section every 3 seconds
function showState() {
	clearTimeout(firstTime);
	clearTimeout(lastTime);
	if (req != null) {
		req.abort();
	}
	req = new XMLHttpRequest();
	req.onreadystatechange = () => {
		// somehow status was 0 on Windows, but "OK" works on both Beken and Windows
		if (req.readyState == 4 && req.statusText == "OK") {
			if (
				!(
				/*
					document.activeElement.tagName == "INPUT" &&
					(document.activeElement.type == "number" || document.activeElement.type == "color")
				*/
				// save some bytes and use regular expression
					(actEl=document.activeElement).tagName == "INPUT" && (/number|color/.test(actEl.type))
				)
			) {
				var stateEl = getElement("state");
				if (stateEl) {
					stateEl.innerHTML = req.responseText;
					var valel = getElement("DATA").dataset;
					getElement("DT").textContent= valel.time > 10 ? Date(valel.time*1e3) : "unset";
					getElement("onlineFor").textContent=valel.online;
				}
			}
			clearTimeout(firstTime);
			clearTimeout(lastTime);
			lastTime = setTimeout(showState, 3e3);
		}
	};
	req.open("GET", "index?state=1", true);
	req.send();
	firstTime = setTimeout(showState, 3e3);
}

function PoorMansNTP() {
	var d=new Date(); 
	return '/pmntp?EPOCH=' + (d/1000|0) + '&OFFSET=' + d.getTimezoneOffset()* - 60;
};

function onLoad() {
	showState();
}

function submitTemperature(slider) {
	var form = getElement("form132");
	var kelvinField = getElement("kelvin132");
	kelvinField.value =(1000000 / slider.value|0)|0;
	form.submit();
}

window.addEventListener("load", onLoad);
// I have modified below to use slice on window.location instead of hardcoded index because it was breaking "back" function of web page
history.pushState(null, "", window.location.pathname.slice(1)); // drop actions like 'toggle' from URL

setTimeout(() => {
	var changedEl = getElement("changed");
	if (changedEl) {
		changedEl.innerHTML = "";
	}
}, 5e3); // hide change info
