//The content of this file get set into ha_discovery_script (new_http.cs)

function send_ha_disc() {
  var xhr = new XMLHttpRequest();
  xhr.open(
    "GET",
    "/ha_discovery?prefix=" + document.getElementById("ha_disc_topic").value,
    false
  );
  xhr.onload = function () {
    if (xhr.status === 200) {
      alert(xhr.responseText);
    } else if (xhr.status === 404) {
      alert("Error invoking ha_discovery");
    }
  };
  xhr.onerror = function () {
    alert("Error invoking ha_discovery");
  };
  xhr.send();
}
