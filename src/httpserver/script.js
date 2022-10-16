//The content of this file get set into htmlHeadStyle (new_http.cs)

var firstTime,
    lastTime,
    req = null;
var onlineFor;
var onlineForEl = null;

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
        if (req.readyState == 4 && req.status == 200) {
            if (
                !(
                    document.activeElement.tagName == "INPUT" &&
                    (document.activeElement.type == "number" || document.activeElement.type == "color")
                )
            ) {
                getElement("state").innerHTML = req.responseText;
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

function fmtUpTime(totalSeconds) {
    var days, hours, minutes, seconds;

    days = Math.floor(totalSeconds / (24 * 60 * 60));
    totalSeconds = totalSeconds % (24 * 60 * 60);
    hours = Math.floor(totalSeconds / (60 * 60));
    totalSeconds = totalSeconds % (60 * 60);
    minutes = Math.floor(totalSeconds / 60);
    seconds = totalSeconds % 60;

    if (days > 0) {
        return `${days} days, ${hours} hours, ${minutes} minutes and ${seconds} seconds`;
    }
    if (hours > 0) {
        return `${hours} hours, ${minutes} minutes and ${seconds} seconds`;
    }
    if (minutes > 0) {
        return `${minutes} minutes and ${seconds} seconds`;
    }
    return `just ${seconds} seconds`;
}

function updateOnlineFor() {
    onlineForEl.textContent = fmtUpTime(++onlineFor);
}

function onLoad() {
    onlineForEl = getElement("onlineFor");
    if (onlineForEl) {
        onlineFor = parseInt(onlineForEl.dataset.initial, 10); //We have some valid value
        if (onlineFor) {
            setInterval(updateOnlineFor, 1000);
        }
    }

    showState();
}

window.addEventListener("load", onLoad);
history.pushState(null, "", "index"); // drop actions like 'toggle' from URL

setTimeout(() => {
    getElement("changed").innerHTML = "";
}, 5e3); // hide change info
