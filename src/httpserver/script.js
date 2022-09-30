var firstTime, lastTime, req=null; 
eb=s=>document.getElementById(s);

// refresh status section every 3 seconds
function showState() { 
    clearTimeout(firstTime);
    clearTimeout(lastTime);
    if (req!=null) { req.abort() }
    req=new XMLHttpRequest();
    req.onreadystatechange=()=>{
        if(req.readyState==4 && req.status==200){
            if (!(document.activeElement.tagName=='INPUT' && 
                (document.activeElement.type=='number' || document.activeElement.type=='color'))) {
                    var s=req.responseText;
                    eb('state').innerHTML=s; 
            }
            clearTimeout(firstTime);
            clearTimeout(lastTime);
            lastTime=setTimeout(showState, 3e3);
    }};
    req.open('GET','index?state=1', true);
    req.send();
    firstTime=setTimeout(showState, 3e3);
}
window.addEventListener('load', showState);
history.pushState(null, '', 'index'); // drop actions like 'toggle' from URL
setTimeout(()=>{eb('changed').innerHTML=''}, 5e3); // hide change info
