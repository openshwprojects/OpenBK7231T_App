let fs = require('fs');

let commands = [];

let inFile = 0;

function mytrim(text){
    text = text.trim();
    if (text.startsWith('"')) text = text.slice(1, -1);
    return text;
}


function getFolder(name, cb){
    console.log('dir:'+name);

    let list = fs.readdirSync(name);
    for (let i = 0; i < list.length; i++){
        let file = name + '/' + list[i];
        let s = fs.statSync(file);

        if (s.isDirectory()){
            getFolder(file, cb);
        } else {
            if (file.toLowerCase().endsWith('.c') || file.toLowerCase().endsWith('.cpp')){
                inFile++;
                console.log('file:'+file);
                let data = fs.readFileSync(file);
                let text = data.toString('utf-8');
                let lines = text.split('\n');
                for (let i = 0; i < lines.length; i++){
                    let line = lines[i].trim();
                    // like CMD_RegisterCommand("SetChannel", "", CMD_SetChannel, "qqqqq0", NULL);

                    if (line.startsWith('CMD_RegisterCommand(')){
                        line = line.slice('CMD_RegisterCommand('.length);
                        parts = line.split(',');
                        let cmd = {
                            name: mytrim(parts[0]),
                            args: mytrim(parts[1]),
                            function: mytrim(parts[2]),
                            userDesc: mytrim(parts[3]),
                            file: file.slice(6),
                        };
                        commands.push(cmd);
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


let md = 
`# Commands

| Command        | Arguments          | Description  | fn | File |
| ------------- |:-------------:| -----:| ------ | ------ |
`;
for (let i = 0; i < commands.length; i++){

    /* like:
    | Command        | Arguments          | Description  | 
    | ------------- |:-------------:| -----:|
    | setChannel     | [ChannelIndex][ChannelValue] | Sets a raw channel to given value. Relay channels are using 1 and 0 values. PWM channels are within [0,100] range. Do not use this for LED control, because there is a better and more advanced LED driver with dimming and configuration memory (remembers setting after on/off), LED driver commands has "led_" prefix. |
    | addChannel     | [ChannelIndex][ValueToAdd][ClampMin][ClampMax] | Ads a given value to the channel. Can be used to change PWM brightness. Clamp min and max arguments are optional. |
    */
    let cmd = commands[i];

    md += `| ${cmd.name} | ${cmd.args} | ${cmd.userDesc} | ${cmd.function} | ${cmd.file} |\n`;

}

md += '\n';

fs.writeFileSync('commands.md', md);

console.log('wrote commands.md');

