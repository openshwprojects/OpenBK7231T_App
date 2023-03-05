let fs = require('fs');

let commands = [];
let cmdindex = {};
let channels = [];
let chanindex = {};

let inFile = 0;

function mytrim(text){
    text = text.trim();
    if (text.startsWith('"')) text = text.slice(1, -1);
    return text;
}


function getFolder(name, cb){
    //console.log('dir:'+name);

    let list = fs.readdirSync(name);
    for (let i = 0; i < list.length; i++){
        let file = name + '/' + list[i];
        let s = fs.statSync(file);

        if (s.isDirectory()){
            getFolder(file, cb);
        } else {
            let sourceFile = file.toLowerCase().endsWith('.c') || file.toLowerCase().endsWith('.cpp');
            let headerFile = file.toLowerCase().endsWith('.h');
            if (sourceFile || headerFile) {
                inFile++;
                //console.log('file:'+file);
                let data = fs.readFileSync(file);
                let text = data.toString('utf-8');
                let lines = text.split('\n');
                let newlines = [];
                let modified = 0;
                for (let i = 0; i < lines.length; i++){
                    let line = lines[i].trim();
                    // like CMD_RegisterCommand("SetChannel", "", CMD_SetChannel, "qqqqq0", NULL);

                    //parse enum starting with "typedef enum channelType_e {"
                    if (headerFile && line.startsWith('typedef enum channelType_e {')) {
                        newlines.push(lines[i]);
                        let j;
                        for (j = i; j < lines.length; j++) {
                            let line2raw = lines[j];
                            let line2 = line2raw.trim();
                            if (line2.startsWith('//')) {
                                newlines.push(line2);
                                continue;
                            }
                            if (line2.startsWith('//chandetail:')) {
                                let commentlines = [];
                                let j2;
                                for (j2 = j; j2 < lines.length; j2++) {
                                    let l = lines[j2].trim();
                                    if (l.startsWith('//chandetail:')) {
                                        l = l.slice(12);
                                        commentlines.push(l);
                                        newlines.push(lines[j2]);
                                    } else {
                                        break;
                                    }
                                }
                                // move our parsing forward to skip all found
                                j = j2;
                                let json = commentlines.join('\n');
                                try {
                                    let chan = JSON.parse(json);
                                    if (chanindex[cmd.name]) {
                                        console.error('duplicate command docs at file: ' + file + ' line: ' + line);
                                        console.error(line);
                                    } else {
                                        channels.push(chan);
                                        chanindex[cmd.name] = chan;
                                    }
                                } catch (e) {
                                    console.error('error in json at file: ' + file + ' line: ' + line);
                                    console.error(json);
                                }
                            } else if (line2.startsWith("ChType_")) {
                                let chanName = line2.substr("ChType_".length);
                                chanName = chanName.replace(/,$/, "");

                                console.log("Channel type name: " + chanName);

                                let chan = {
                                    name: chanName,
                                    descr: mytrim("TODO"),
                                    enum: "ChType_"+chanName,
                                    file: file.slice(6),
                                    driver: mytrim(""),
                                };


                                if (!chanindex[chan.name]) {
                                    // it did not have a doc line before
                                    let json = JSON.stringify(chan);
                                    // insert CR at "fn":
                                    json = json.split('"descr":');
                                    json = json.join('\n"descr":');
                                    json = json.split('"enum":');
                                    json = json.join('\n"enum":');
                                    json = json.split('"file":');
                                    json = json.join('\n"file":');
                                    json = json.split('"driver":');
                                    json = json.join('\n"driver":');
                                    let jsonlines = json.split('\n');
                                    for (let j = 0; j < jsonlines.length; j++) {
                                        jsonlines[j] = '\t//chandetail:' + jsonlines[j];
                                    }
                                    newlines.push(...jsonlines);
                                    modified++;
                                    channels.push(chan);
                                    chanindex[chan.name] = chan;
                                }
                                newlines.push(lines[j]);
                            }
                            if (line2.endsWith('} channelType_t;')) {
                                //newlines.push(line2raw);
                                break;
                            }
                        }
                        i = j;
                    }
                    if (line.startsWith('//cmddetail:')){
                        let commentlines = [];
                        let j;
                        for (j = i; j < lines.length; j++){
                            let l = lines[j].trim();
                            if (l.startsWith('//cmddetail:')){
                                l = l.slice(12);
                                commentlines.push(l);
                                newlines.push(lines[j]);
                            } else {
                                break;
                            }
                        }
                        // move our parsing forward to skip all found
                        i = j;
                        let json = commentlines.join('\n');
                        try{
                            let cmd = JSON.parse(json);
                            if (cmdindex[cmd.name]){
                                console.error('duplicate command docs at file: '+file+' line: '+line);
                                console.error(line);
                            } else {
                                commands.push(cmd);
                                cmdindex[cmd.name] = cmd;
                            }
                        } catch(e) {
                            console.error('error in json at file: '+file+' line: '+line);
                            console.error(json);
                        }
                    }

                    // i may have changed...
                    line = lines[i].trim();

                    if (line.startsWith('CMD_RegisterCommand(')){
                        line = line.slice('CMD_RegisterCommand('.length);
                        parts = line.split(',');
                        //cmddetail:{"name":"SetChannel", "args":"TODO", "fn":"CMD_SetChannel", "descr":"qqqqq0", "example":"", "file":"");

                        let cmd = {
                            name: mytrim(parts[0]),
                            args: mytrim(parts[1]),
                            descr: mytrim(""),
                            fn: mytrim(parts[2]),
                            file: file.slice(6),
                            requires:"",
                            examples: "",
                        };

                        //if (cmd.descr !== 'NULL'){
                        //    console.log('replace "'+cmd.descr+'" with NULL');
                         //   lines[i] = lines[i].replace(', NULL, NULL);', ', NULL);');
                         //   modified++;
                        //}

                        if (!cmdindex[cmd.name]){
                            // it did not have a doc line before
                            let json = JSON.stringify(cmd);
                            // insert CR at "fn":
                            json = json.split('"descr":');
                            json = json.join('\n"descr":');
                            json = json.split('"fn":');
                            json = json.join('\n"fn":');
                            json = json.split('"examples":');
                            json = json.join('\n"examples":');
                            let jsonlines = json.split('\n');
                            for (let j = 0; j < jsonlines.length; j++){
                                jsonlines[j] = '\t//cmddetail:'+jsonlines[j];
                            }
                            newlines.push(... jsonlines);
                            modified++;
                            commands.push(cmd);
                            cmdindex[cmd.name] = cmd;
                        }
                    }

                    newlines.push(lines[i]);
                }
                if (modified){
                    let newdata = newlines.join('\n');
                    try{
                        fs.writeFileSync(file, newdata);
                        console.log('updated '+file);
                    } catch (e){
                        console.error('failed to update '+file);
                    }
                }

                inFile--;

                if (!inFile){
                    //if (cb) cb();
                }
            }
        }
    }
}


console.log('starting');

getFolder('./src');


let mdshort = 
`# Commands
Here is the latest, up to date command list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| Command        | Arguments          | Description  |
|:------------- |:------------- | -----:|
`;

let mdlong = 
`# Commands
Here is the latest, up to date command list.
This file was autogenerated by running 'node scripts/getcommands.js' in the repository.
All descriptions were taken from code.
Do not add anything here, as it will overwritten with next rebuild.
| Command        | Arguments          | Description  | Location |
|:------------- |:-------------:|:----- | ------:|
`;

for (let i = 0; i < commands.length; i++){

    /* like:
    | Command        | Arguments          | Description  | 
    | ------------- |:-------------:| -----:|
    | setChannel     | [ChannelIndex][ChannelValue] | Sets a raw channel to given value. Relay channels are using 1 and 0 values. PWM channels are within [0,100] range. Do not use this for LED control, because there is a better and more advanced LED driver with dimming and configuration memory (remembers setting after on/off), LED driver commands has "led_" prefix. |
    | addChannel     | [ChannelIndex][ValueToAdd][ClampMin][ClampMax] | Ads a given value to the channel. Can be used to change PWM brightness. Clamp min and max arguments are optional. |
    */
    let cmd = commands[i];

    let textshort = `| ${cmd.name} | ${cmd.args}${cmd.requires?'\nReq:'+cmd.requires:''} | ${cmd.descr}${cmd.examples?'\ne.g.:'+cmd.examples:''} |`;
    let textlong = `| ${cmd.name} | ${cmd.args}${cmd.requires?'\nReq:'+cmd.requires:''} | ${cmd.descr}${cmd.examples?'\ne.g.:'+cmd.examples:''} | File: ${cmd.file}\nFunction: ${cmd.fn} |`;

    // allow multi-row entries in table entries.
    textshort = textshort.replace(/\n/g, '<br/>');
    textlong = textlong.replace(/\n/g, '<br/>');

    mdshort += textshort;
    mdshort += '\n';
    mdlong += textlong;
    mdlong += '\n';
}

mdshort += '\n';
mdlong += '\n';

const dirPath = 'docs/json';
if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true });
}

fs.writeFileSync(`${dirPath}/commands.json`, JSON.stringify(commands, null, 2));
console.log('wrote json/commands.json');
fs.writeFileSync('docs/commands.md', mdshort);
console.log('wrote commands.md');
fs.writeFileSync('docs/commands-extended.md', mdlong);
console.log('wrote commands-extended.md');

