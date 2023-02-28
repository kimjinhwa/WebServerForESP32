"use strict"
//const warningImage = new Image();

const warningSound = document.getElementById('warning-sound');
warningSound.loop = true;
warningSound.muted = false;
warningSound.autoplay = true;
var alarmStatus = false;
var connectUrl = "192.168.0.57"

warningSound.load();
function ValidateIPaddress(inputText) {
    let ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    if (inputText.match(ipformat)) {
        return true;
    }
    else {
        alert("입력값의 형식이 잘못 되었습니다.!");
        return false;
    }
}


function getCookie(name) {
    var nameEQ = name + '=';
    var cookies = document.cookie.split(';');
    for (var i = 0; i < cookies.length; i++) {
        var cookie = cookies[i];
        while (cookie.charAt(0) == ' ') {
            cookie = cookie.substring(1, cookie.length);
        }
        if (cookie.indexOf(nameEQ) == 0) {
            return cookie.substring(nameEQ.length, cookie.length);
        }
    }
    return null;
}
function setCookie(name, value, expires) {
    var cookie =
        name +
        '=' +
        value +
        '; expires=' +
        expires.toUTCString() +
        '; path=/';
    document.cookie = cookie;
}
async function loadSound() {
    //await new Promise(resolve => setTimeout(resolve, 1000));
    let playPromise = warningSound.play()
        .then(() => {
            console.log("sound loaded OK")
        })
        .catch((error) => {
            console.log(`error ${error} `)
            //loadSound()
        });
}
loadSound();//

let register12 = 0b0000111000000000;
let register13 = 0b0000000000000011;
let register14 = 0b0000000000000000;
let register15 = 0b0000010000000000;
let obj = {};
Object.defineProperty(obj, 'register12', {
    get: function () {
        return register12;
    },
    set: function (value) {
        if (register12 === value) { }
        //console.log(`value=${value}`);
        else {
            //여기서 윈속을 전송한다
            register12 = value;
            const event = new CustomEvent('register12Updated');
            window.dispatchEvent(event);
            //console.log("이벤트발생")
        }
        //inputVoltage.innerHTML = value;
    }
});
Object.defineProperty(obj, 'register13', {
    get: function () {
        return register13;
    },
    set: function (value) {
        if (register13 === value) { }
        //console.log(`value=${value}`);
        else {
            register13 = value;
            const event = new CustomEvent('register13Updated');
            window.dispatchEvent(event);
        }
    }
});
Object.defineProperty(obj, 'register14', {
    get: function () {
        return register14;
    },
    set: function (value) {
        if (register14 === value) { }
        //console.log(`value=${value}`);
        else {
            register14 = value;
            const event = new CustomEvent('register14Updated');
            window.dispatchEvent(event);
        }
    }
});
Object.defineProperty(obj, 'register15', {
    get: function () {
        return register15;
    },
    set: function (value) {
        if (register15 === value) { }
        //console.log(`value=${value}`);
        else {
            register15 = value;
            const event = new CustomEvent('register15Updated');
            window.dispatchEvent(event);
        }
    }
});


//Function to set the alarm status and trigger the event listener
function fireAlarmStatus(status) {
    //drawdiag.warningImage(status);
    try {
        drawdiag.warningAlarm(status);
        //console.log(`fireAlarmStatus(${status})`)
    } catch (e) {

    }
    //drawdiag.warningImage(event.detail);
    //alarmStatus = status;
    // let alarmStatusEvent = new CustomEvent('alarmStatusChanged', { detail: status });
    // document.dispatchEvent(alarmStatusEvent);
}
//const modBusDataArray = [/* log data received from UPS monitoring system */];


class drawDiagram {
    constructor(x, y) {
        this.flashInterval;
        this.alarmImgStatus = false;
        this.interval;
        this.wireColor = '#3d8021';
        this.diagram = [];
        this.POWER_LINE_FAIL = 1 < 11;
        this.width = 850,
            this.height = 330;
        this.bat_w = 70,
            this.bat_h = 45;
        this.draw = SVG().addTo('#drawing').size(this.width, this.height);
        this.drawTime;

        this.start_x = x;
        this.start_y = y + 30;
        this.powerLineWidth = 15,
            this.PL_1_length = 60,
            this.PL_2_length = 40,
            this.PL_3_length = 40;
        this.background = this.draw.rect(this.width, this.height).fill('#E3E8E6');
        window.addEventListener('register12Updated', () => {
            for (let i = 0; i < 16; i++) {
                let idName12 = `12regbit_${i}`;
                if (document.getElementById(idName12)) {
                    //console.log(`${15 - i}checked = ${document.getElementById(idName12).checked }`)
                    document.getElementById(idName12).checked = obj.register12 & 1 << 15 - i;
                }
            }
            drawdiag.commandDraw();
        });
        window.addEventListener('register13Updated', () => {
            for (let i = 0; i < 16; i++) {
                let idName13 = `13regbit_${i}`;

                //console.log(`${15 - i} checked ${document.getElementById(idName13).checked}`)

                if (document.getElementById(idName13)) {
                    document.getElementById(idName13).checked = obj.register13 & 1 << 15 - i;
                }
            }
            drawdiag.commandDraw();
        });
        window.addEventListener('register14Updated', () => {
            for (let i = 0; i < 16; i++) {
                let idName14 = `14regbit_${i}`;

                //console.log(`${15 - i}checked`)

                if (document.getElementById(idName14)) {
                    document.getElementById(idName14).checked = obj.register14 & 1 << 15 - i;
                }
            }
            drawdiag.commandDraw();
        });
        window.addEventListener('register15Updated', () => {
            for (let i = 0; i < 16; i++) {
                let idName15 = `15regbit_${i}`;

                //console.log(`${15 - i}checked`)

                if (document.getElementById(idName15)) {
                    document.getElementById(idName15).checked = obj.register15 & 1 << 15 - i;
                }
            }
            drawdiag.commandDraw();
        });
    }
    drawSymbol() {
        ////PL_1
        this.drawTime = this.draw
            .text('2023-02-21 19:30:02')
            .move(10, 10)
            .stroke({ width: 1, color: 'blue' })
        let PL_1 = this.draw.symbol().add(
            this.draw
                .rect(this.PL_1_length, this.powerLineWidth)
                .stroke({ width: 1, color: 'black' })
                .move(this.start_x, this.start_y)

        )
            .add(
                this.draw
                    .text('AC 입력')
                    .move(10, +145)
                    .stroke({ width: 1, color: 'black' })
                    .font({ size: 14 })
            )


        let pl_1 = this.draw.use(PL_1).fill('none')

        // 0
        this.diagram.push({ name: pl_1, color: this.wireColor });

        this.start_x = this.start_x + this.PL_1_length;
        let CB_symbol_1 = this.draw.symbol()
            .add(
                this.draw
                    .path('M+20' + ' ' + this.start_y + ' v-15h-15v-12h45v12h-15v15z')
                    .stroke({ width: 1, color: 'black' })
                    .move(this.start_x - 15, this.start_y - 13)
            )
            .add(
                this.draw
                    .text('CB1')
                    .move(68, 95)
                    .stroke({ width: 1, color: 'black' })
                    .font({ size: 14 })
            )
        let cb_1 = this.draw.use(CB_symbol_1)
            .fill('none')
            .move(0, obj.register12 & 1 << 9 ? -15 : 0)
        // 1 PL_2
        this.diagram.push({ name: cb_1, color: 'none' });
        this.start_y -= this.powerLineWidth - 15;
        this.start_x += this.powerLineWidth;
        let PL_2 =
            this.draw
                .symbol()
                .add(
                    this.draw
                        .rect(this.PL_2_length, this.powerLineWidth)
                        .stroke({ width: 1, color: 'black' })
                        .move(this.start_x, this.start_y)
                );
        let pl_2 = this.draw.use(PL_2).fill('none')
        //2 
        this.diagram.push({ name: pl_2, color: 'none' });
        this.start_x = this.start_x + this.PL_2_length;
        //converter
        let converTer = this.draw.symbol();
        converTer.add(
            this.draw
                .rect(this.bat_w, this.bat_h)
                .stroke({ width: 1, color: 'black' })
                .move(this.start_x, this.start_y + this.powerLineWidth / 2 - this.bat_h / 2)
        );

        converTer.add(
            this.draw
                .line(
                    this.start_x,
                    this.start_y + this.powerLineWidth / 2 + this.bat_h / 2,
                    this.start_x + this.bat_w,
                    this.start_y + this.powerLineWidth / 2 - this.bat_h / 2
                )
                .stroke({ width: 1, color: 'black' })
        );
        converTer.add(
            this.draw
                .text('~')
                .stroke({ width: 1, color: 'black' })
                .move(this.start_x + this.bat_w / 5, this.start_y - this.bat_w / 4)
                .scale(2)
        );
        converTer.add(
            this.draw
                .text('_')
                .stroke({ width: 1, color: 'black' })
                .move(this.start_x + this.bat_w / 1.6, this.start_y - this.bat_w / 5)
                .scale(2)
        );

        let batteryConverter = this.draw.use(converTer).fill(this.wireColor);
        //3
        this.diagram.push({ name: batteryConverter, color: 'none' });

        this.start_x += this.bat_w;
        let PL_3 = (
            this.draw
                .symbol()
                .add(
                    this.draw
                        .rect(this.PL_3_length, this.powerLineWidth)
                        .stroke({ width: 1, color: 'black' })
                        .move(this.start_x, this.start_y)
                )
        );
        let pl_3 = this.draw.use(PL_3).fill('none')
        //4
        this.diagram.push({ name: pl_3, color: 'none' });

        this.start_x = this.start_x - 29;

        let CB_symbol_2 = this.draw.symbol()
            .add(
                this.draw
                    .path('M+20' + ' ' + this.start_y + ' v-15h-15v-12h45v12h-15v15z')
                    .stroke({ width: 1, color: 'black' })
                    .move(this.start_x - 111, this.start_y - 13)
            )
            .add(
                this.draw
                    .text('CB3')
                    .move(68, 95)
                    .stroke({ width: 1, color: 'black' })
                    .font({ size: 14 })
            )
        let cb_3 = this.draw
            .use(CB_symbol_2)
            .move(this.start_x, 0)
            .fill('none');
        //5
        this.diagram.push({ name: cb_3, color: 'none' });

        this.start_x = this.start_x + this.powerLineWidth + 70;
        let pathString = '';
        let paths = `M${this.start_x} ${this.start_y}h50v-60h140v15h-125v180h80v15h-95v-135h-50 z`;

        var PL_4 =
            this.draw
                .symbol()
                .add(this.draw.path(paths).stroke({ width: 1, color: 'black' }))
        let pl_4 = this.draw.use(PL_4).fill('none')//.hide()
        //6
        this.diagram.push({ name: pl_4, color: 'none' });
        //CB_3 battery 
        let CB_symbol_3 = this.draw.symbol()
            .add(
                this.draw
                    .path('M+20' + ' ' + this.start_y + ' v-15h-15v-12h45v12h-15v15z')
                    .stroke({ width: 1, color: 'black' })
                    .move(this.start_x - 196, this.start_y - 13)
            )
            .add(
                this.draw
                    .text('CB2')
                    .move(68, 95)
                    .stroke({ width: 1, color: 'black' })
                    .font({ size: 14 })
            )
        let cb_2 = this.draw.use(CB_symbol_3).move(this.start_x, this.start_y).fill('none');
        //7 
        this.diagram.push({ name: cb_2, color: 'none' });
        //PL_BAT
        let PL_BAT = this.draw
            .symbol()
            .rect(60, 15)
            .stroke({ width: 1, color: 'black' })

        let pl_bat = this.draw.use(PL_BAT).fill(this.wireColor);
        //8
        this.diagram.push({ name: pl_bat, color: 'none' });

        var batSymbol = this.draw.symbol();
        batSymbol.add(
            this.draw
                .rect(70, 50)
                .stroke({ width: 2, color: 'black' })
                .move(470, 235)
        );
        batSymbol.add(
            this.draw
                .line(480, 255, 530, 255)
                .stroke({ width: 8, color: 'black' })
        )
        batSymbol.add(
            this.draw
                .line(490, 270, 520, 270)
                .stroke({ width: 4, color: 'black' })
        )
            .add(
                this.draw
                    .text('BAT')
                    .move(550, 250)
                    .stroke({ width: 1, color: 'black' })
                    .font({ size: 16 })
            )
            ;
        let bat_symbol = this.draw.use(batSymbol).fill('none')
        //9 
        this.diagram.push({ name: bat_symbol, color: 'none' });

        var sid_draw = this.draw.symbol()
            .add(
                this.draw.rect(30, 30)
                    .stroke({ width: 1, color: 'black' })
                    .move(this.start_x + 90, this.start_y - 55)
            )
            .add(
                this.draw
                    .path('M350 100 v-20l10 10zv-10 ')
                    .stroke({ width: 1, color: 'black' })
                    .fill('black')
            )
            .add(
                this.draw
                    .path('M350 90 h-5m15 0 h5 m-5 -10 v20')
                    .fill('black')
                    .stroke({ width: 1, color: 'black' })
            )
            .add(
                this.draw
                    .text('SID')
                    .move(348, 105)
                    .stroke({ width: 1, color: 'black' })
                    .font({ size: 9 })
            )

        let sid_1 = this.draw.use(sid_draw)
            .move(this.start_x - 122, this.start_y - 142).fill('none').scale(2.8, 2);
        //10
        this.diagram.push({ name: sid_1, color: 'none' });
        let pl_sid = this.draw.rect(100, 15)
            .stroke({ width: 1, color: 'black' })
            .move(this.start_x + 276, this.start_y - 60)
            .fill('none')
        //11
        this.diagram.push({ name: pl_sid, color: 'none' });


        let CB_symbol_4 = this.draw.symbol()
            .add(
                this.draw
                    .path('M+20' + ' ' + this.start_y + ' v-15h-15v-12h45v12h-15v15z')
                    .stroke({ width: 1, color: 'black' })
                    .move(this.start_x - 196, this.start_y - 13)
            )
            .add(
                this.draw
                    .text('CB4')
                    .move(68, 95)
                    .stroke({ width: 1, color: 'black' })
                    .font({ size: 14 })
            )
        let cb_4 = this.draw.use(CB_symbol_4).fill('none');
        //12
        this.diagram.push({ name: cb_4, color: 'none' });

        let pl_cb4_out = this.draw
            .rect(100, 15)
            .stroke({ width: 1, color: 'black' })
            .move(this.start_x + 392, this.start_y - 60)
            .fill('none')
            .add(
        )
        //13
        this.draw
            .text('DC')
            .move(this.start_x + 500, this.start_y - 63)
            .stroke({ width: 1, color: 'black' })
            .font({ size: 14 })
        this.diagram.push({ name: pl_cb4_out, color: 'none' });
    }

    //hello(){}
    commandDraw() {     /* 전원이 충전기 이거나 배터리 쪽에서 입력이 되고 있다면  */
        let isPowered = (obj.register15 & 1 << 10 || obj.register12 & 1 << 10);
        let cb1_status = obj.register12 & 1 << 9;
        let cb2_status = obj.register12 & 1 << 10;
        let cb3_status = obj.register12 & 1 << 11;
        this.diagram[0].name.fill(obj.register14 & 1 << 11 ? "none" : this.wireColor);
        if (cb1_status) { //CB1의 상태가 ON이면 
            this.diagram[1].name.fill(this.diagram[0].name.fill()); //정전이면 앞의 라인의 상태에 따른다.
        }
        else  //off 이면
        {
            this.diagram[1].name.fill('none');
        }
        this.diagram[1].name.move(0, cb1_status ? 0 : -18);
        //CB_1 후단의 라인 
        this.diagram[2].name.fill(this.diagram[1].name.fill());
        //충전기의 상태 1. 우선은 앞의 상태를 받아 온다
        //그리고 나서 충전기 정지 상태 비트를 본다
        this.diagram[3].name.fill(this.diagram[2].name.fill());
        if (!(obj.register15 & 1 << 10))
            this.diagram[3].name.fill('none');
        else
            this.diagram[3].name.fill(this.wireColor);
        //PL_1 -> CB_1 -> CONVERTER -> PL_3
        //충전기가 정지 상태이면  
        //충전기 후단의 라인이다.
        if (isPowered && cb3_status)
            this.diagram[4].name.fill(this.wireColor);
        else
            this.diagram[4].name.fill('none');
        //PL_1 -> CB_1 -> CONVERTER -> PL_3->cb_3
        if (cb3_status) { //CB2의 상태가 ON이면 
            if (isPowered) this.diagram[5].name.fill(this.wireColor);
            else this.diagram[5].name.fill('none');
            this.diagram[5].name.move(this.start_x - 85, this.start_y - 130);
            //console.log(this.diagram[5].name.bbox().x)
        }
        else {
            this.diagram[5].name.fill('none');
            this.diagram[5].name.move(this.start_x - 85, this.start_y - 130 - 18);
        }
        //this.diagram[5].name.fill('black');
        //PL_1 -> CB_1 -> CONVERTER -> PL_3->CB2->PL_4
        //PL_4는 앞의 조건과 축전지의 조건 2가지를 같아 봐야 한다. 
        // 우선은 앞의 조건을 먼저 살펴보자. 
        //CB_3가 닫혀 있다면 cb_3의 컬러를 그대로 사용한다. 
        //CB_3가 열려 있다면 Battery라인을 봐야 한다.
        //메인 파워 라인이다
        if (isPowered && (cb2_status || cb3_status)) { //CB3의 상태가 ON이면 
            this.diagram[6].name.fill(this.wireColor);
        }
        else {
            this.diagram[6].name.fill('none');
        }
        //this.diagram[6].name.fill('black');

        //PL_1 -> CB_1 -> CONVERTER -> PL_3->CB2->cb_3
        if (obj.register12 & 1 << 10) { //CB2의 상태 즉 축전지가 연결되어 있으면 
            this.diagram[7].name
                .fill(this.wireColor)
                .move(this.start_x + 76, this.start_y + 5);
        }
        else {
            this.diagram[7].name
                .fill('none')
                .move(this.start_x + 76, this.start_y + 5 - 18);
        }
        //pl_bat
        this.diagram[8].name.fill(this.wireColor)
            .move(this.start_x + 160, this.start_y + 135);
        //this.diagram[8].name.fill('black')

        //9 battery
        this.diagram[9].name.fill('#9be4c7')
            .move(0, 10)
        //10 sid_1 중앙 파워라인을 따른다
        this.diagram[10].name.fill(this.diagram[6].name.fill());
        //11 pl_sid 
        this.diagram[11].name.fill(this.diagram[6].name.fill());
        //12 cb_4

        ;

        //12 cb_4
        if (obj.register13 & 1 << 0) { //CB2의 상태 즉 축전지가 연결되어 있으면 
            this.diagram[12].name
                .fill(this.diagram[11].name.fill())
                .move(557, -60);
        }
        else {
            this.diagram[12].name
                .fill('none')
                .move(557, -60 - 18);
        }
        //13 pl_cb4_line
        if (obj.register13 & 1 << 0) { //CB4가 닫혀 있다면 그 앞의 상태를 따른다. 
            this.diagram[13].name
                .fill(this.diagram[11].name.fill())
        }
        else
            this.diagram[13].name
                .fill('none');
        // this.alarmImage = this.draw
        //     .image('alert.svg', function (event) { })
        //     .move(700, 220)
        //     .size(70, 70)
        //     .click(function () {
        //     })
        //     .hide()

        // let startTime = Date.now();
        //.animate({ duration: 2000, ease: '<>', delay: 0, repeat: true })
        //.opacity(1)
        //.dmove(50, 50)
        // alarmImage = this.draw
        //     .image('alert.svg', function (event) { })
        //     .move(700, 220)
        //     .size(70, 70)
        //     .hide()
    }
    warningImage(showImage) {
        console.log(`this.alarmImage.show()`)
        if (showImage) {
            //alarmImage.show();
            if (!alarmStatus) {
                alarmStatus = true;
                alarmImage.show();
                this.flashInterval = setInterval(() => {
                    alarmImage.show();
                    //console.log(`this.alarmImage.show()`)
                    setTimeout(() => {
                        alarmImage.hide();
                        //console.log(`this.alarmImage.hide()`)
                    }, 500)
                }, 1000)

            }
        }
        else if (!showImage) {
            alarmStatus = false;
            clearInterval(this.flashInterval)
            alarmImage.hide()
        }
    }
    warningAlarm(warning) {
        if (warning) {
            try {
                warningSound.play()
                    .then((resolve) => {
                        console.log("sound load ok")
                    })
            }
            catch (e) {
                console.log(e);
            }
        }
        warningSound.muted = !warning;
    }
}

// SVG.on(document,'DOMContentLoaded',function(){
//             alert("");
//         }
// )
class modBusDataClass {
    constructor(dataArrayBuffer, justOnlyEvent, alarmEvent, modBusDataArray) {
        this.wireColor = '#3d8021';

        this.justOnlyEvent = justOnlyEvent;
        this.alarmEvent = alarmEvent;
        this.modBusDataArray = modBusDataArray;

        const int16Array = new Uint8Array(dataArrayBuffer);
        const arrayBuffer = int16Array.buffer;
        const dataView = new DataView(arrayBuffer);
        this.logTime = dataView.getUint32(0, 1);
        this.modBus60 = [];
        this.stringfyModBus60 = null;// variable to store the previous modBus60 state
        this.prveInden = -1;

        for (let i = 0; i < 60; i++) {
            this.modBus60[i] = dataView.getInt16(4 + (2 * i), 1);
        }
        this.Register_12 = this.modBus60[12];
        this.Register_13 = this.modBus60[13];
        this.Register_14 = this.modBus60[14];
        this.Register_15 = this.modBus60[15];
        this.prevRegister_12 = 0;
        this.prevRegister_13 = 0;
        this.prevRegister_14 = 0;
        this.prevRegister_15 = 0;

        //만일 이전에 이벤트와 비교해서 변경된것이 없다면 아무짓도 하지 않고 리턴한다.
        if (modBusDataArray.length > 0) {
            this.prevIndex = modBusDataArray.length - 1;
            this.prevRegister_12 = modBusDataArray[this.prevIndex].Register_12;
            this.prevRegister_13 = modBusDataArray[this.prevIndex].Register_13;
            this.prevRegister_14 = modBusDataArray[this.prevIndex].Register_14;
            this.prevRegister_15 = modBusDataArray[this.prevIndex].Register_15;
            if (this.prevRegister_12 ^ this.Register_12 ||
                this.prevRegister_13 ^ this.Register_13 ||
                this.prevRegister_14 ^ this.Register_14 ||
                this.prevRegister_15 ^ this.Register_15
            ) {
                //console.log("log changed " + this.prevRegister_12);
                this.processModBus60();
                modBusDataArray.push(this);
            }
        }
        else {
            this.prevIndex = - 1;
            //this.processModBus60();
            modBusDataArray.push(this);
        }
    }
    addData(dataArrayBuffer) {
        const int8Array = new Uint8Array(dataArrayBuffer);
        const arrayBuffer = int8Array.buffer;
        const dataView = new DataView(arrayBuffer);
        this.logTime = dataView.getUint32(0, 1) * 1000;
        //console.log(`logTime(${this.logTime}) =  ${new Date((this.logTime))}`)

        for (let i = 0; i < 60; i++) {
            this.modBus60[i] = dataView.getInt16(4 + (2 * i), 1);
        }

        //만일 이전에 이벤트와 비교해서 변경된것이 없다면 아무짓도 하지 않고 리턴한다.
        if (this.modBusDataArray.length > 0) {
            this.prevIndex = this.modBusDataArray.length - 1;
            this.prevRegister_12 = this.modBusDataArray[this.prevIndex].Register_12;
            this.prevRegister_13 = this.modBusDataArray[this.prevIndex].Register_13;
            this.prevRegister_14 = this.modBusDataArray[this.prevIndex].Register_14;
            this.prevRegister_15 = this.modBusDataArray[this.prevIndex].Register_15;

            this.Register_12 = this.modBus60[12];
            this.Register_13 = this.modBus60[13];
            this.Register_14 = this.modBus60[14];
            this.Register_15 = this.modBus60[15];

            if (this.prevRegister_12 ^ this.Register_12 ||
                this.prevRegister_13 ^ this.Register_13 ||
                this.prevRegister_14 ^ this.Register_14 ||
                this.prevRegister_15 ^ this.Register_15
            ) {
                //console.log("log changed " + this.prevRegister_12);
                //console.log(`this.alarmEvent.length ${this.alarmEvent.length}`)
                let prevAlarmCount = this.alarmEvent.length;
                this.processModBus60();
                let nowAlarmCount = this.alarmEvent.length;
                //console.log(`this.alarmEvent.length ${this.alarmEvent.length}`)
                //html 로그테이블이 존재 하면 데이타를 추가한다. 
                let table = document.getElementById("AlarmTable");
                if (nowAlarmCount > prevAlarmCount) {
                    if (table != null) {
                        let newRow = table.insertRow(1); // Insert a new row at index 0
                        let cell1 = newRow.insertCell(0); // Insert a new cell in the new row
                        let cell2 = newRow.insertCell(1); // Insert another new cell in the new row
                        let logDate = new Date(this.logTime);
                        cell1.innerHTML = `${logDate.getFullYear()}/${logDate.getMonth() + 1}/${logDate.getDate()} ${logDate.getHours()}:${logDate.getMinutes()}:${logDate.getSeconds()} `;
                        if (this.alarmEvent[this.alarmEvent.length - 1].events) {
                            this.alarmEvent[this.alarmEvent.length - 1].events.forEach(etdata => {
                                //console.log(etdata);
                                cell2.innerHTML += etdata;//this.justOnlyEvent.length; // Set the content of the second cell
                                cell2.innerHTML += '<br>'
                            })
                        }
                    }

                }
                //console.log(ev.logTime)
                // ev.events.forEach(etdata => {
                //     //td.innerHTML += etdata + "<br>";
                //     console.log(etdata)
                // });
                // console.log(ev.events)

                table = document.getElementById("EventTable");
                if (table != null) {
                    let newRow = table.insertRow(1); // Insert a new row at index 0
                    let cell1 = newRow.insertCell(0); // Insert a new cell in the new row
                    let cell2 = newRow.insertCell(1); // Insert another new cell in the new row
                    //cell1.appendChild(document.createTextNode("new data1"));
                    //cell2.appendChild(document.createTextNode("new data2"));
                    let logDate = new Date(this.logTime);
                    cell1.innerHTML = `${logDate.getFullYear()}/${logDate.getMonth() + 1}/${logDate.getDate()} ${logDate.getHours()}:${logDate.getMinutes()}:${logDate.getSeconds()} `;
                    // console.log(this.justOnlyEvent.length)
                    // console.log(this.justOnlyEvent[this.justOnlyEvent.length - 1].events);
                    this.justOnlyEvent[this.justOnlyEvent.length - 1].events.forEach(etdata => {
                        //console.log(etdata);
                        cell2.innerHTML += etdata;//this.justOnlyEvent.length; // Set the content of the second cell
                    })
                    //console.log(this)
                }
                this.modBusDataArray.push(this);
            }
        }
    }
    // method to process the modBus60 register and get the changes
    // 이미 여기와 들어 왔다는 것은 데이타가 기존의 데이타와는 다른 새로운 이벤트가 발행 핬다는 것이다.
    // 우리는 기존과 어떤 값이 바뀌었는지를 알아내야 한다
    // 이것을 하기위하여 exclusive OR를 사용하면 어떤 비트의 값이 변경되었는지를 알수 있고 
    // 그리고 지금의 값이 0 혹은 1인지를 확인하여 메세지를 + 하여 그 값을 출력한다. 
    // 해당되는 BIT는 다시 해당 BIT를 & 연산하여 값이 1 혹은 0인지를 판단한다.
    processModBus60() {
        const bitToCheck_12_event = [0, 2, 3, 5, 6, 8, 9, 10, 11, 12, 13, 14]; // bits to check for specific conditions
        const bitToCheck_13_event = [0, 1, 5, 6, 8];
        const bitToCheck_14_event = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];
        const bitToCheck_15_event = [1, 9, 10];
        const eventList = [];
        const alarmList = [];
        //const bitCheck_12131415=[bitToCheck_12_event,bitToCheck_13_event,bitToCheck_14_event,bitToCheck_15_event ];
        const bitCheck_12131415 = [bitToCheck_12_event, bitToCheck_13_event, bitToCheck_14_event, bitToCheck_15_event];
        let nowPos = 12;
        bitCheck_12131415.forEach(bitToCheck => {
            // check each bit for specific condition
            let checkRegister_12 = this.Register_12 ^ this.prevRegister_12;
            let checkRegister_13 = this.Register_13 ^ this.prevRegister_13;
            let checkRegister_14 = this.Register_14 ^ this.prevRegister_14;
            let checkRegister_15 = this.Register_15 ^ this.prevRegister_15;
            bitToCheck.forEach((bitIndex) => {
                //console.log(`checkRegister_12:${checkRegister_12}`)
                const bitValue_12 = checkRegister_12 & (1 << bitIndex);// 12번지에 셋팅되어 있는 비트에 맞는 메세지를 가져온다.   
                const bitValue_13 = checkRegister_13 & (1 << bitIndex);// 12번지에 셋팅되어 있는 비트에 맞는 메세지를 가져온다.   
                const bitValue_14 = checkRegister_14 & (1 << bitIndex);// 12번지에 셋팅되어 있는 비트에 맞는 메세지를 가져온다.   
                const bitValue_15 = checkRegister_15 & (1 << bitIndex);// 12번지에 셋팅되어 있는 비트에 맞는 메세지를 가져온다.   
                //이제 bitValue_12가 1이면 bitIndex에 해당 하는 비트가 변경되었다는 뜻이다. 
                // 그리고 현재의 값이 O인지 1인지에 따른 메세지를 출력하자 
                //console.log(`bitValue_12:${bitValue_12}`)
                const nowBitValue_12 = this.Register_12 & (1 << bitIndex);
                const nowBitValue_13 = this.Register_13 & (1 << bitIndex);
                const nowBitValue_14 = this.Register_14 & (1 << bitIndex);
                const nowBitValue_15 = this.Register_15 & (1 << bitIndex);
                let message_12, message_13, message_14, message_15;

                if (nowPos === 12)
                    message_12 = this.getMessage(bitIndex, bitValue_12, nowBitValue_12, nowPos);
                else if (nowPos === 13)
                    message_13 = this.getMessage(bitIndex, bitValue_13, nowBitValue_13, nowPos);
                else if (nowPos === 14)
                    message_14 = this.getMessage(bitIndex, bitValue_14, nowBitValue_14, nowPos);
                else if (nowPos === 15)
                    message_15 = this.getMessage(bitIndex, bitValue_15, nowBitValue_15, nowPos);
                //if (message) console.log(`message :${message.message}`)
                if (message_12) {
                    // add message to event or alarm list based on condition
                    //모든 이벤트는 기록한다. 단 alarm은 alarm만 기록한다
                    eventList.push(message_12.message); // if exgist, all event must recored.
                    if (message_12.type === 'alarm') {  //alarm으로 기록된 것만 여기에 기록한다.
                        alarmList.push(message_12.message);
                    }
                }
                if (message_13) {
                    eventList.push(message_13.message); // if exgist, all event must recored.
                    if (message_13.type === 'alarm') {  //alarm으로 기록된 것만 여기에 기록한다.
                        alarmList.push(message_13.message);
                    }
                }
                if (message_14) {
                    eventList.push(message_14.message); // if exgist, all event must recored.
                    if (message_14.type === 'alarm') {  //alarm으로 기록된 것만 여기에 기록한다.
                        alarmList.push(message_14.message);
                    }
                }
                if (message_15) {
                    //console.log(`message_15.message ${message_15.type} ${message_15.message}`)
                    eventList.push(message_15.message); // if exgist, all event must recored.
                    if (message_15.type === 'alarm') {  //alarm으로 기록된 것만 여기에 기록한다.
                        alarmList.push(message_15.message);
                    }
                }
            });
            // add log time and event/alarm list to appropriate array only if there are changes
            if (eventList.length > 0 || alarmList.length > 0) {
                const newModBus60 = JSON.stringify(this.modBus60); // get the stringified modBus60
                //console.log(newModBus60);
                if (this.stringfyModBus60 === null || this.stringfyModBus60 !== newModBus60) {
                    // save the changes only if there are no previous data or the current data is different from the previous data
                    if (eventList.length > 0) {
                        this.justOnlyEvent.push({ logTime: this.logTime, events: eventList });
                    }
                    if (alarmList.length > 0) {
                        this.alarmEvent.push({ logTime: this.logTime, events: alarmList });
                    }
                    this.stringfyModBus60 = newModBus60;
                }
            }
            nowPos++;
        });
    }

    // method to get message based on bit value and index
    getMessage(bitIndex, bitValue, nowBitValue, nowPos) {
        //trigger :0 if set to 1
        //trigger :1 if set 0 to 1 
        //trigger :2 if set 1 to 0 

        const messages_12 = {
            0: { type: 'alarm', message: '컨버터 과온', trigger: 0, ON: "경보발생", OFF: "->정상" },//(0: 정상 , 1: 과온경보)	경보, 이벤트
            2: { type: 'event', message: '입력 MC 상태 이벤트', trigger: 0, ON: "->열림", OFF: "->닫힘" },
            3: { type: 'alarm', message: 'Main FUSE Fail(0: 정상 , 1: 이상)	경보, 이벤트', trigger: 0, ON: "열림->닫힘", OFF: "->열림" },
            5: { type: 'alarm', message: 'Pluse Ground Fault ', trigger: 0, ON: "발생", OFF: "해제" },
            6: { type: 'alarm', message: 'Minus Ground Fault ', trigger: 0, ON: "발생", OFF: "해제" },
            8: { type: 'alarm', message: '축전지 저전압 이상', trigger: 0, ON: "경보", OFF: "해제" },
            9: { type: 'alarm', message: 'CB1 상태 ', trigger: 2, ON: "열림->닫힘", OFF: " 열림경보" },
            10: { type: 'alarm', message: 'CB2 상태 ', trigger: 2, ON: "열림->닫힘", OFF: " 열림경보" },
            11: { type: 'alarm', message: 'CB3 상태 ', trigger: 2, ON: "열림->닫힘", OFF: " 열림경보" },
            12: { type: 'alarm', message: 'Converter_R_GDU Fault ', trigger: 0, ON: "경보발생", OFF: "경보발생해제" },
            13: { type: 'alarm', message: 'Converter_S_GDU Fault ', trigger: 0, ON: "경보발생", OFF: "경보발생해제" },
            14: { type: 'alarm', message: 'Converter_T_GDU Fault ', trigger: 0, ON: "경보발생", OFF: "경보발생해제" }
        };
        //const bitToCheck_13_event = [ 0, 1, 5, 6,  8 ];
        const messages_13 = {
            0: { type: 'alarm', message: 'CB4 상태 ', trigger: 2, ON: "열림->닫힘)", OFF: "열림경보" },
            1: { type: 'alarm', message: 'CB5 상태 ', trigger: 2, ON: "열림->닫힘)", OFF: "열림경보" },
            2: { type: 'alarm', message: 'FAN Fault ', trigger: 0, ON: "이상발생", OFF: "->해재" },
            5: { type: 'alarm', message: '입력 과전류 ', trigger: 0, ON: "이상발생", OFF: "->정상회복" },
            6: { type: 'alarm', message: '배터리 과전압 ', trigger: 0, ON: "이상발생", OFF: "->정상회복" },
            8: { type: 'alarm', message: '배터리 저전압 ', trigger: 0, ON: "이상발생", OFF: "->정상회복" }
        };
        //const bitToCheck_14_event = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];
        const messages_14 = {
            1: { type: 'alarm', message: '충전기 전류제한 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            2: { type: 'alarm', message: '배터리 충전전류제한 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            3: { type: 'alarm', message: '배터리 과전압제한 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            4: { type: 'alarm', message: '배터리 저전압제한이상 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            5: { type: 'alarm', message: '입력 상회전 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            6: { type: 'alarm', message: '입력 저전압 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            7: { type: 'alarm', message: '입력 과전압 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            8: { type: 'alarm', message: '전력 모듈', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            9: { type: 'alarm', message: '입력 주파수 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            10: { type: 'alarm', message: '입력 비동기 ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            11: { type: 'alarm', message: '정전 ', trigger: 0, ON: "발생", OFF: "정전 후 복전" },
            12: { type: 'alarm', message: '입력 MC ', trigger: 0, ON: "이상발생", OFF: "이상발생후 회복" },
            13: { type: 'alarm', message: '결상', trigger: 0, ON: "발생", OFF: "발생후 회복" },
            14: { type: '', message: '', trigger: 0, ON: "복전", OFF: "" }
        };
        //const bitToCheck_15_event = [1,9,10];
        const messages_15 = {
            1: { type: 'alarm', message: '과부하 정지 ', trigger: 0, ON: "발생", OFF: "후 회복" },
            9: { type: 'alarm', message: '과부하 ', trigger: 0, ON: "시간제한이상발생 ", OFF: "시간제한이상발생후 회복 " },
            10: { type: 'alarm', message: '충전기 ', trigger: 2, ON: "운전중", OFF: "정지상태" }
        };

        let message;
        if (nowPos === 12)
            message = messages_12[bitIndex];
        else if (nowPos === 13)
            message = messages_13[bitIndex];
        else if (nowPos === 14)
            message = messages_14[bitIndex];
        else if (nowPos === 15)
            message = messages_15[bitIndex];
        //if (this.prevIndex === -1) //처음이면 prebitr은 고려하지 않는다.
        if (message) {
            if (message.trigger === 0) {
                if (bitValue) { //이 부분이 변경되어야 한다. 
                    // 메세지는 있고, bitValue 가 설정이 되었다면 해당 비트가 셋팅되었다.
                    // 이제 0과 1에 따라 메세지가 발송 되어야 한다. 메세지를 보내야 한다.
                    //console.log(`bitIndex:${bitIndex} bitValue ${bitValue_12} nowBitValue ${nowBitValue}`)
                    return { type: message.type = nowBitValue ? message.type : 'event', message: message.message = nowBitValue ? message.message + message.ON : message.message + message.OFF };
                }
            }
            else if (message.trigger === 1) {
                if (bitValue) {
                    return { type: message.type, message: message.message = nowBitValue ? message.message + message.ON : message.message + message.OFF };
                }
            }
            else if (message.trigger === 2) { //2 인 얘들은 닫힘 이벤트가 발생했을 경우에는 type 이 이벤트로 바꾸자
                if (bitValue) {
                    //nowBitValue
                    //console.log(`bitIndex:${bitIndex} bitValue ${bitValue} nowBitValue ${nowBitValue}`)
                    message.type = nowBitValue ? 'event' : message.type
                    message.message = nowBitValue ? message.message + message.ON : message.message + message.OFF
                    //console.log(`message.type=${message.type},message.message= ${message.message}`)
                    return { type: message.type, message: message.message };
                }
            }
        }
        return null;
    }
}

class modbusDataArrayClass {
    constructor() {

        this.modBusDataArray = [];// log data received from UPS monitoring system 
        this.justOnlyEvent = []; // array to store event data
        this.alarmEvent = []; // array to store alarm data
        this.int8Array = new Uint8Array(124);
        this.arrayBuffer = this.int8Array.buffer;
        this.dataView = new DataView(this.arrayBuffer);

        this.dataView.setUint32(0, (new Date()).getTime() / 1000, 1);
        //console.log((new Date()).getTime() / 1000);
        //console.log(this.dataView.getUint32(0, 1));
        this.dataView.setInt16(4 + 24, 0b0000111000000000, 1);
        this.dataView.setInt16(4 + 26, 0b0000000000000011, 1);
        this.dataView.setInt16(4 + 28, 0b0000000000000000, 1);
        this.dataView.setInt16(4 + 30, 0b0000010000000000, 1);
        this.logTime = this.dataView.getUint32(0, 1);
        // 새로운 인스턴스를 만들지만 만들어지 cmdBus는 Array에 들어가지 
        // 않을 수 도 있다. 앞의 데이타와 동일하면 추가 되지 않는다.
        this.cmodBus = new modBusDataClass(this.dataView.buffer, this.justOnlyEvent, this.alarmEvent, this.modBusDataArray);

        document.addEventListener('alarmStatusChanged', function (event) {
            //console.log('event.detail ${event.detail}' + event.detail)
            // drawdiag.warningAlarm(event.detail);
            // drawdiag.warningImage(event.detail);
        });
    };

    // addEventListener(eventname, callback) {
    //     document.addEventListener(eventname, callback);
    // }
    // setAlarmStatus(status) {
    //     //alarmStatus = status;
    //     //let alarmStatusEvent = new CustomEvent('alarmStatusChanged', { detail: status });
    //     //document.dispatchEvent(alarmStatusEvent);
    // };
    addDataArray(dataArrayBuffer,
        event12 = 0b0000111000000000
        , event13 = 0b0000000000000011
        , event14 = 0b0000000000000000
        , event15 = 0b0000010000000000
    ) {
        this.int8Array = new Uint8Array(dataArrayBuffer);
        this.arrayBuffer = this.int8Array.buffer;

        this.dataView = new DataView(this.arrayBuffer);


        this.dataView.setUint32(0, (this.dataView.getUint32(0, 1) - 9 * 3600), 1);
        //console.log(`logTime(${this.dataView.getUint32(0, 1)}) =  ${new Date((this.dataView.getUint32(0, 1))*1000)}`)
        this.dataView.setInt16(4 + 24, event12, 1);
        this.dataView.setInt16(4 + 24 + 2, event13, 1);
        this.dataView.setInt16(4 + 24 + 2 + 2, event14, 1);
        this.dataView.setInt16(4 + 24 + 2 + 2 + 2, event15, 1);
        this.logTime = this.dataView.getUint32(0, 1);

        //데이타가 추가될 경우 Alarm을 자동으로 설정하고 끈다
        const alarmBitMask_12 = 0b0111111101101001;//[0, 2, 3, 5, 6, 8, 9, 10, 11, 12, 13, 14]; // bits to check for specific conditions
        const alarmBitMask_13 = 0b0000000101100011;//[0, 1, 5, 6, 8];
        const alarmBitMask_14 = 0b0111111111111110;//[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];
        const alarmBitMask_15 = 0b0000011000000010;//[1, 9, 10];
        //if(alarmBitMask_12 & event12 || alarmBitMask_13 & event13 || alarmBitMask_14 & event14 || alarmBitMask_15 & event15 )
        let event12_alarm = event12;
        let event13_alarm = event13;
        let event14_alarm = event14;
        let event15_alarm = event15;
        //9,10,11을 뒤집는다
        event12_alarm = (event12_alarm & (1 << 9)) ? event12_alarm & ~(1 << 9) : event12_alarm | (1 << 9);
        event12_alarm = (event12_alarm & (1 << 10)) ? event12_alarm & ~(1 << 10) : event12_alarm | (1 << 10);
        event12_alarm = (event12_alarm & (1 << 11)) ? event12_alarm & ~(1 << 11) : event12_alarm | (1 << 11);
        //0,1을 뒤집는다
        event13_alarm = (event13_alarm & (1 << 0)) ? event13_alarm & ~(1 << 0) : event13_alarm | (1 << 0);
        event13_alarm = (event13_alarm & (1 << 1)) ? event13_alarm & ~(1 << 1) : event13_alarm | (1 << 1);
        //14을 뒤집는다
        //event14_alarm = (event14_alarm & (1 << 14)) ? event14_alarm & ~(1 << 14) : event14_alarm | (1 << 14);
        // || alarmBitMask_14 & event14_alarm
        event15_alarm = (event15_alarm & (1 << 10)) ? event15_alarm & ~(1 << 10) : event15_alarm | (1 << 10);
        if (alarmBitMask_12 & event12_alarm || alarmBitMask_13 & event13_alarm || alarmBitMask_14 & event14_alarm || alarmBitMask_15 & event15_alarm) {
            //this.setAlarmStatus(true); // Turn on the alarm
            //console.log(`fireAlarmStatus occured`)
            // let stopButton = document.getElementById('showButton');
            // stopButton.dispatchEvent(new Event('click'));
            fireAlarmStatus(true);
        }
        else {
            //this.setAlarmStatus(false); // Turn on the alarm
            //console.log(`fireAlarmStatus released`)
            fireAlarmStatus(false);
            // let stopButton = document.getElementById('stopButton');
            // stopButton.dispatchEvent(new Event('click'));
        }

        this.cmodBus.addData(this.dataView.buffer);
        //this.cmodBus = new modBusDataClass(this.dataView.buffer, this.justOnlyEvent, this.alarmEvent, this.modBusDataArray);
    }


    printTableContent(eventAlarmData, tbody, tr, td) {
        eventAlarmData.forEach(ev => {
            //console.log(ev.events);
            tr = document.createElement("tr");
            td = document.createElement("td");
            td.innerHTML = `${(new Date(ev.logTime * 1000)).getFullYear()}/${(new Date(ev.logTime * 1000)).getMonth() + 1}/${(new Date(ev.logTime * 1000)).getDay()} ${(new Date(ev.logTime * 1000)).getHours()}:${(new Date(ev.logTime * 1000)).getMinutes()}:${(new Date(ev.logTime * 1000)).getSeconds()} `;
            td.style.border = "1px solid black";
            tr.appendChild(td);
            td = document.createElement("td");
            ev.events.forEach(etdata => {
                td.innerHTML += etdata + "<br>";
            });
            td.style.border = "1px solid black";
            tr.appendChild(td);
            tbody.appendChild(tr);

        });

    };
    printEvent(eventAlarmData, heading) {
        // let logHeading = document.createElement("h1");
        // logHeading.innerHTML = heading;


        // modData.printEvent(modData.alarmEvent.reverse(), "Alarm");
        // modData.printEvent(modData.justOnlyEvent.reverse(), "Event");
        let logDiv;
        if (heading === "Alarm")
            logDiv = document.getElementById('alarmDiv')
        //.appendChild(logHeading);
        else
            logDiv = document.getElementById('eventDiv')
        //.appendChild(logHeading);

        //document.getElementById('HtmlLogView').appendChild(logHeading);
        let table = document.createElement("table");
        //AlarmTable EventTable
        table.setAttribute("id", heading + "Table");
        table.style.border = "1px solid black";
        var thead = document.createElement("thead");
        var tr = document.createElement("tr");

        var th = document.createElement("th");
        th.innerHTML = "Time";
        th.style.border = "1px solid black";
        tr.appendChild(th);

        var th = document.createElement("th");
        th.innerHTML = "Event Data";
        th.style.border = "1px solid black";
        tr.appendChild(th);


        thead.appendChild(tr);
        table.appendChild(thead);
        var tr, td;
        var tbody = document.createElement("tbody");

        this.printTableContent(eventAlarmData, tbody, tr, td);
        //this.printTableContent(this.justOnlyEvent, tbody, tr, td);

        table.appendChild(tbody);
        //document.body.appendChild(table);
        //document.getElementById('HtmlLogView').appendChild(table);
        logDiv.appendChild(table);
        //document.getElementById('HtmlLogView').style.display= "none";
    }
}

var isDownloadComplete = false;
var socketCommand = "";
async function isLogFunction(event) {
    let receiveData = ""
    if (event.data === "log_download") {
        //로그파일을 전송하라고 명령을 준다
        socketCommand = "log_download_start";
        receiveData = "";

        let table = document.getElementById("AlarmTable");
        const numRows = table.rows.length;
        for (let i = numRows - 1; i >= 1; i--) {
            // Check if the row index is greater than or equal to the insertion index (1 in this example)
            if (i >= 1) {
                // Delete the row
                table.deleteRow(i);
            }
        }
        let table1 = document.getElementById("EventTable");
        let numRows1 = table1.rows.length;
        for (let i = numRows1 - 1; i >= 1; i--) {
            // Check if the row index is greater than or equal to the insertion index (1 in this example)
            if (i >= 1) {
                // Delete the row
                table1.deleteRow(i);
            }
        }
        //console.log("LogFile request start...");
        return 0;
    }
    if (event.data === "download_complete") {
        console.log("LogFile download complete..." + typeof (event.data));

        isDownloadComplete = true;
        socketCommand = "";
        new Promise((resolve) => {
            setTimeout(resolve, 500);
        }).then(() => {

            //printLogToHtml();
            // for (let i = 0; i < modBusDataArray.length; i++) {
            //     console.log("modBusDataArray.length is " + modBusDataArray.length);
            //     console.log(modBusDataArray[i].logTime);
            //     console.log(modBusDataArray[i].modBus60);
            // }

        });
        //processReceivedData(receiveData );
        return 0;
    }

    if (socketCommand === "log_download_start" || typeof (event.data) == Object) {
        //파일데이타가 도착했다.
        //console.log("LogFile downloading..." + typeof (event.data));
        let reader = new FileReader();
        reader.readAsArrayBuffer(event.data);
        await new Promise(resolve => {
            reader.onload = (evt) => {
                let int8Array = new Uint8Array(evt.target.result);
                let int16Array = new Uint16Array(evt.target.result);
                //obj.register12 = Reg_12; obj.register13 = Reg_13; obj.register14 = Reg_14; obj.register15 = Reg_15;
                modData.addDataArray(int8Array, int16Array[12 + 2], int16Array[13 + 2], int16Array[14 + 2], int16Array[15 + 2]);//
                //console.log(`12=${int16Array[12+2]}  13=${int16Array[13+2]} 14=${int16Array[14+2]} 15=${int16Array[15+2]}    `)
                //modBusDataArray.push(new modBusDataClass(evt.target.result));
                //console.log("modBusDataArray.length is " + modBusDataArray.length);
                resolve();
            }
        });
        receiveData += event.data;
        return 0;
    }
    return 1;
}
var isPingReceive = true;
var pingStartTime = new Date().getTime()
var pingEndTime = new Date().getTime() + 3000;
var retryCount = 0;
setInterval(() => {
    pingEndTime = new Date().getTime()
    if (pingStartTime + 2 * 1000 < pingEndTime) {
        //2초 이내에 여기에 들어 온다
        //이제 반드시 isPingReceive true가 되어 있어야 한다
        if (isPingReceive === true)
            console.log('network alive')
        else {
            statusBar.style = "color: red;"
            statusBar.innerHTML = "네트웍 재 접속중..." + retryCount++
            webSocket = null
            let newWebSocket = new myWebSocket('ws://' + connectUrl + ':81/');
            webSocket = newWebSocket; // assign the new WebSocket instance to the original variable

        }
    }
    if (webSocket.readyState === webSocket.OPEN) {
        let data = JSON.stringify({ 'command_type': 'ping' });
        isPingReceive = false;
        pingStartTime = new Date().getTime()
        webSocket.send(data)
    }
    if (webSocket.readyState !== webSocket.OPEN) {
    }
}, 3000);
function setWebSocketOnEvent(webSocket) {
    webSocket.addEventListener('open', (event) => {
        setWebSocketOnEvent(webSocket)
        console.log("WebSocket connection opened");
        statusBar.style = "color: blue;";
        statusBar.innerHTML = "네트웍 접속 완료";
        retryCount = 0
    })
    webSocket.addEventListener('error', (event) => {
        console.log("WebSocket error:", error);
    })

    // webSocket.addEventListener('close', (event) => {
    //     console.log(`socket Close from ${connectUrl}`);
    //     console.log('Socket retry connect....');
    //     setTimeout(() => {
    //         statusBar.style = "color: red;"
    //         statusBar.innerHTML = "네트웍 접속 끊김 ...재 접속중..."
    //         webSocket = null;
    //         let newWebSocket = new myWebSocket('ws://' + connectUrl + ':81/');
    //         webSocket = newWebSocket; // assign the new WebSocket instance to the original variable
    //     }, 1000);
    // })

    webSocket.addEventListener('message', async (event) => {
        isPingReceive = true;
        var ret = await isLogFunction(event)
        if (ret === 1) {
            try {
                let data = JSON.parse(event.data);
                //console.log(`data.command_type ${typeof (data)} == ${data.command_type}`)
                if (data.command_type == 'nowtime') {
                    winsocksubData.showTime(data);
                }
                else if (data.command_type == 'pong') {
                    console.log(`get pong net work alive`)
                }
                else if (data.command_type == 'modbus') {
                    winsocksubData.fileHtmlData(data);
                }
                else if (data.command_type == 'timeSet') {
                    statusBar.style = "color: blue;"
                    statusBar.innerHTML = ""
                    for (let key in data) {
                        statusBar.innerHTML += `${key}=${data[key]}  `
                        //console.log(`${key}=${data[key]}`)
                    };

                }
                else if (data.command_type == 'time') {
                    //alert(message)
                    console.log(event.data)
                    statusBar.innerHTML = `request command='${data.command_type}' answer received`
                    statusBar.style = "color: blue;"
                    ntp1Txt.value = data.ntp1
                    ntp2Txt.value = data.ntp2
                    ntpuseChkbox.checked = data.ntpuse
                    statusbar.innerHTML = data.message
                }
                else if (data.command_type == 'ipaddress') {
                    statusBar.innerHTML = `request command='${data.command_type}' answer received`
                    statusBar.style = "color: blue;"
                    IpAddresstxt.value = data.ipaddress
                    subnetMaskTxt.value = data.subnetmask
                    gatewayTxt.value = data.gateway
                    webSocketServerTxt.value = data.websocketserver
                    webSocketPortTxt.value = data.webSocketPort
                    ntpuseChkbox.checked = data.ntpuse

                }
                else {
                    statusBar.innerHTML = `request command='${data.command_type}' answer received`
                    statusBar.style = "color: blue;"
                    console.log(`ret value ${ret}`)
                    console.log(event.data)
                }


            }
            catch (error) {
                //webSocket
                webSocket.close();
                webSocket = null;
                setTimeout(() => {
                    const newWebSocket = new myWebSocket('ws://' + connectUrl + ':81/');
                    webSocket = newWebSocket; // assign the new WebSocket instance to the original variable
                }, 1000);
                console.error(error);
            }
            //console.log(`data received ${data.command_type}`)
        }
    })


    // webSocket.onopen = function(event) {
    // };
}
class myWebSocket extends WebSocket {
    constructor(url) {
        super(url); // call super() first
        this.modbus_registor = {
            "time": 0,
            "value": [
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            ],
        };
        setWebSocketOnEvent(this);
    }

    fileHtmlData(data) {
        let int16Array = new Uint16Array(data.value);
        let int8Array = new Uint8Array(int16Array.length * 2 + 4);
        for (let i = 0; i < int16Array.length; i++) {
            int8Array[4 + i * 2] = int16Array[i] & 0xFF; // lower byte
            int8Array[4 + i * 2 + 1] = (int16Array[i] >> 8) & 0xFF; // upper byte
        }
        obj.register12 = data.value[12];
        obj.register13 = data.value[13];
        obj.register14 = data.value[14];
        obj.register15 = data.value[15];
        modData.addDataArray(int8Array, data.value[12], data.value[13], data.value[14], data.value[15]);//
        this.modbus_registor.value = data.value;
        let el = document.getElementById('modbusData');
        el.innerHTML = 'Received Data: ';
        el.innerHTML = data.value;
        inputVol_R.innerHTML = data.value[20];
        inputVol_S.innerHTML = data.value[21];
        inputVol_T.innerHTML = data.value[22];
        inputAmp_R.innerHTML = data.value[23];
        inputAmp_S.innerHTML = data.value[24];
        inputAmp_T.innerHTML = data.value[25];
        batVoltage.innerHTML = data.value[29];
        batAmpere.innerHTML = data.value[30];
        loadRate.innerHTML = data.value[32] + "(%)";
    }
    showTime(data) {
        //console.log(`data.time ${data.time}`)
        let nowTime = new Date(data.time * 1000);
        drawdiag.drawTime.text(nowTime.toLocaleString());
    }
}
var webSocket = new myWebSocket('ws://' + connectUrl + ':81/');

class winsocksubClass {
    constructor() {
        this.modbus_registor = {
            "time": 0,
            "value": [
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            ],
        };
    }
    fileHtmlData(data) {
        let int16Array = new Uint16Array(data.value);
        let int8Array = new Uint8Array(int16Array.length * 2 + 4);
        for (let i = 0; i < int16Array.length; i++) {
            int8Array[4 + i * 2] = int16Array[i] & 0xFF; // lower byte
            int8Array[4 + i * 2 + 1] = (int16Array[i] >> 8) & 0xFF; // upper byte
        }
        obj.register12 = data.value[12];
        obj.register13 = data.value[13];
        obj.register14 = data.value[14];
        obj.register15 = data.value[15];
        modData.addDataArray(int8Array, data.value[12], data.value[13], data.value[14], data.value[15]);//
        this.modbus_registor.value = data.value;
        let el = document.getElementById('modbusData');
        el.innerHTML = 'Received Data: ';
        el.innerHTML = data.value;
        inputVol_R.innerHTML = data.value[20];
        inputVol_S.innerHTML = data.value[21];
        inputVol_T.innerHTML = data.value[22];
        inputAmp_R.innerHTML = data.value[23];
        inputAmp_S.innerHTML = data.value[24];
        inputAmp_T.innerHTML = data.value[25];
        batVoltage.innerHTML = data.value[29];
        batAmpere.innerHTML = data.value[30];
        loadRate.innerHTML = data.value[32] + "(%)";
    }
    showTime(data) {
        //console.log(`data.time ${data.time}`)
        let nowTime = new Date(data.time * 1000);
        drawdiag.drawTime.text(nowTime.toLocaleString());
    }
}
let winsocksubData = new winsocksubClass()
var eventsss
function addEventArray() {
    let Reg_12 = 0x00; let Reg_13 = 0x00; let Reg_14 = 0x00; let Reg_15 = 0x00;
    let sendData;
    for (let i = 0; i < 16; i++) {
        let idName12 = `12regbit_${i}`;
        let idName13 = `13regbit_${i}`;
        let idName14 = `14regbit_${i}`;
        let idName15 = `15regbit_${i}`;
        let idName = [idName12, idName13, idName14, idName15]
        idName.forEach((ide) => {
            let chk = document.getElementById(ide);
            //console.log("------" + ide);
            chk.addEventListener('click', (e) => {
                eventsss = e
                Reg_12 = 0x00; Reg_13 = 0x00; Reg_14 = 0x00; Reg_15 = 0x00;
                let iddName12; let iddName13; let iddName14; let iddName15;
                for (let j = 0; j < 16; j++) {
                    iddName12 = `12regbit_${j}`; iddName13 = `13regbit_${j}`; iddName14 = `14regbit_${j}`; iddName15 = `15regbit_${j}`;
                    if (document.getElementById(iddName12).checked) Reg_12 |= 1 << 15 - j;
                    if (document.getElementById(iddName13).checked) Reg_13 |= 1 << 15 - j;
                    if (document.getElementById(iddName14).checked) Reg_14 |= 1 << 15 - j;
                    if (document.getElementById(iddName15).checked) Reg_15 |= 1 << 15 - j;
                }
                if (e.target.id.includes('12regbit')) {
                    //console.log('12regbit' + "   click")
                    sendData = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 12, 'set': Reg_12 }); /* BIT 12 =0 열림*/
                }
                if (e.target.id.includes('13regbit')) {
                    //console.log('13regbit' + "   click")
                    sendData = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 13, 'set': Reg_13 }); /* BIT 13 =0 열림*/
                }
                if (e.target.id.includes('14regbit')) {
                    //console.log('14regbit' + "   click")
                    sendData = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 14, 'set': Reg_14 }); /* BIT 14 =0 열림*/
                }
                if (e.target.id.includes('15regbit')) {
                    //console.log('15regbit' + "   click")
                    sendData = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 15, 'set': Reg_15 }); /* BIT 15 =0 열림*/
                }
                if (webSocket.readyState === webSocket.OPEN)
                    webSocket.send(sendData);
                else console.log("socket was closed");

                let int8Array = new Uint8Array(124);
                let arrayBuffer = int8Array.buffer;
                let dataView = new DataView(arrayBuffer);
                dataView.setUint32(0, (new Date()).getTime() / 1000 + 9 * 3600, 1);
                int8Array = dataView.buffer
                obj.register12 = Reg_12; obj.register13 = Reg_13; obj.register14 = Reg_14; obj.register15 = Reg_15;
                console.log('------------------here')
                modData.addDataArray(int8Array, Reg_12, Reg_13, Reg_14, Reg_15);//
            });
        });
    }
}



async function testClassCode() {
    //modData.beep();
    let int8Array = new Uint8Array(124);
    //modData.addDataArray(int8Array, 0b0000111000000000);
    //await new Promise(resolve => setTimeout(resolve, 1000));
    obj.register12 = 0b0000111000000000;
    obj.register13 = 0b0000000000000011;
    obj.register14 = 0b0000000000000000;
    obj.register15 = 0b0000010000000000;
    //modData.addDataArray(int8Array, obj.register12, obj.register13, obj.register14, obj.register15);//
    //아래의 프린트는 틀만 프린트 하고 
    // 실제 데이타는 이벤트 데이타 처리 루틴에서 이루어 진다
    modData.printEvent(modData.alarmEvent.reverse(), "Alarm");
    modData.printEvent(modData.justOnlyEvent.reverse(), "Event");
    //modData.printAlarm("AlarmLog");
}

// const showButton = document.querySelector('#showButton');
// const stopButton = document.querySelector('#stopButton');

//12regbit_0
document.getElementById('showButton').addEventListener('click', (e) => {
    fireAlarmStatus(true);
});
document.getElementById('stopButton').addEventListener('click', (e) => {
    console.log("click")
    fireAlarmStatus(false);
});
document.getElementById('viewLog').addEventListener('click', (e) => {
    console.log("viewLog click")
    document.getElementById('HtmlMainView').style.display = 'none';
    document.getElementById('HtmlLogView').style.display = 'grid';
    document.getElementById('viewBasic').style.display = 'none';
    document.getElementById('sectionNetworkInfo').style.display = 'none';
});
document.getElementById('htmlMain').addEventListener('click', (e) => {
    console.log("viewLog click")
    document.getElementById('HtmlMainView').style.display = 'block';
    document.getElementById('HtmlLogView').style.display = 'none';
    document.getElementById('viewBasic').style.display = 'none';
    document.getElementById('sectionNetworkInfo').style.display = 'none';
});
document.getElementById('viewB').addEventListener('click', (e) => {
    console.log("viewLog click")
    document.getElementById('HtmlMainView').style.display = 'none';
    document.getElementById('HtmlLogView').style.display = 'none';
    document.getElementById('viewBasic').style.display = 'block';
    document.getElementById('sectionNetworkInfo').style.display = 'none';
});
document.getElementById('viewN').addEventListener('click', (e) => {
    console.log("viewLog click")
    document.getElementById('HtmlMainView').style.display = 'none';
    document.getElementById('HtmlLogView').style.display = 'none';
    document.getElementById('viewBasic').style.display = 'none';
    document.getElementById('sectionNetworkInfo').style.display = 'grid';
});
//logout
document.getElementById('logout').addEventListener('click', (e) => {
    setCookie("login", "", new Date(0));
    window.location.href = "login.html";
});
document.getElementById('timesetbtn').addEventListener('click', (e) => {
    //ValidateIPaddress(document.getElementById('timesetbtn').value)
});


function downloadCSV(tableId) {
    let table = document.getElementById(tableId);
    let rows = table.rows;
    let csvContent = "";

    // Loop through rows
    for (let i = 0; i < rows.length; i++) {
        let cells = rows[i].cells;

        // Loop through cells in a row
        for (let j = 0; j < cells.length; j++) {
            // Get cell value and add to CSV content
            let cellValue = cells[j].textContent.trim();
            if (j !== cells.length - 1) {
                csvContent += encodeURI(cellValue) + ",\t";
            } else {
                csvContent += encodeURI(cellValue) + "\n";
            }
        }
    }

    // Prompt user to choose filename and location
    let filename = tableId
    // Create Blob object
    let link = document.createElement("a");
    csvContent = "%EF%BB%BF" + csvContent
    link.href = "data:text/csv;charset=utf-8," + (csvContent);
    link.download = filename + ".csv";
    link.click();
}

document.getElementById('saveAlarmBtn').addEventListener('click', (e) => {
    downloadCSV("AlarmTable");
});
document.getElementById('saveEventBtn').addEventListener('click', (e) => {
    downloadCSV("EventTable");
});


document.getElementById('btnQuery1').addEventListener('click', (e) => {
    let data = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 12, 'set': 0b0000111000000000 }); /* BIT 12 =0 열림*/
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
})
document.getElementById('timesettxt').value = new Date().toLocaleString()

document.getElementById('btnLogRead').addEventListener('click', (e) => {
    console.log("log_download")
    webSocket.send("log_download");
});

document.getElementById('getServerIpAddress').addEventListener('click', (e) => {
    let commandData = `ipaddress`;
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});
document.getElementById('IpAddressbtn').addEventListener('click', (e) => {
    let text = document.getElementById('IpAddresstxt')
    if (!ValidateIPaddress(text.value)) {
        text.value = "0.0.0.0" + text.value;
        return
    };
    let ipaddresss = text.value;
    let commandData = `ipaddress -s -ipaddr ${ipaddresss}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});
//
document.getElementById('subnetBtn').addEventListener('click', (e) => {
    let text = document.getElementById('subnetMaskTxt')
    if (!ValidateIPaddress(text.value)) {
        return
    };
    let ipaddresss = text.value;
    let commandData = `ipaddress -s -subnetmask ${ipaddresss}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});
document.getElementById('gatewayBtn').addEventListener('click', (e) => {
    let text = document.getElementById('gatewayTxt')
    if (!ValidateIPaddress(text.value)) {
        return
    };
    let ipaddresss = text.value;
    let commandData = `ipaddress -s -gateway ${ipaddresss}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});

document.getElementById('ntp1Btn').addEventListener('click', (e) => {
    let ntpAddress = ntp1Txt.value;
    let commandData = `time -s -ntp1 ${ntpAddress}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});
document.getElementById('ntp2Btn').addEventListener('click', (e) => {
    let ntpAddress = ntp2Txt.value;
    let commandData = `time -s -ntp2 ${ntpAddress}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});
document.getElementById('socketServerBtn').addEventListener('click', (e) => {
    let text = document.getElementById('webSocketServerTxt')
    if (!ValidateIPaddress(text.value)) {
        return
    };
    let ipaddresss = text.value;
    let commandData = `ipaddress -s -websocket ${ipaddresss}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});
document.getElementById('socketportBtn').addEventListener('click', (e) => {
    let text = document.getElementById('webSocketPortTxt')
    let ipaddresss = text.value;
    let commandData = `ipaddress -s -socketport ${ipaddresss}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});
document.getElementById('ntpuseBtn').addEventListener('click', (e) => {
    let text = document.getElementById('ntpuseChkbox')
    let checked = text.checked ? 1 : 0;
    let commandData = `time -s -ntpuse ${checked}`
    let data = JSON.stringify({ 'command_type': commandData });
    if (webSocket.readyState === webSocket.OPEN)
        webSocket.send(data);
    else console.log("socket was closed");
});


document.getElementById('timesetbtn').addEventListener('click', (e) => {
    let nowTime = new Date();
    const data = JSON.stringify({ 'command_type': 'timeSet', 'reg': 0, 'set': nowTime.getTime() / 1000 }); /* BIT 12 =0 열림*/
    console.log(data);
    webSocket.send(data);
})

// let data = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 12, 'set': register12 }); /* BIT 12 =0 열림*/
// if (webSocket.readyState === webSocket.OPEN)
//     webSocket.send(data);
// else console.log("socket was closed");

//event = new CustomEvent('click');
//window.dispatchEvent(event);

addEventArray(); //배포본에는 사용되지 않을 펑션이다.
const drawdiag = new drawDiagram(9, 100);

//drawdiag  클래스가 생성된후 사용해야 한다
var draw = drawdiag.draw;
var alarmImage = draw
    .image('alert.svg', function (event) { })
    .move(700, 220)
    .size(70, 70)
    .click(function () { })
    .hide()
//drawdiag.animate(true)
drawdiag.drawSymbol();
drawdiag.commandDraw();
drawdiag.warningAlarm(false);
const modData = new modbusDataArrayClass();
//var ipv4_address = $('#ipv4');
// ipv4_address.inputmask({
//     alias: "ip",
//     greedy: false //The initial mask shown will be "" instead of "-____".
// });
window.onload = function () {
    //console.log("onLoad");
    let loginCookie = getCookie("login");
    // If the cookie exists and is not expired, the user is logged in
    if (loginCookie && new Date() < new Date(loginCookie)) {
        console.log("login ok")
        //document.getElementById('HtmlMainView').style.display = 'grid';
        document.getElementById('HtmlMainView').style.display = 'none';
        //document.getElementById('HtmlLogView').style.display = 'grid';
        document.getElementById('HtmlLogView').style.display = 'none';
        document.getElementById('viewBasic').style.display = 'none';
        document.getElementById('testRoutine').style.display = 'block';
        document.getElementById('sectionNetworkInfo').style.display = 'grid';
        testClassCode();
    }
    else {
        window.location.href = "login.html";
        return false;
    }
    // If the cookie does not exist or is expired, the user is not logged in
};