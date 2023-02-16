"use strict"
//const modBusDataArray = [/* log data received from UPS monitoring system */];
class modBusDataClass {
    constructor(dataArrayBuffer, justOnlyEvent, alarmEvent, modBusDataArray) {

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
    // method to process the modBus60 register and get the changes
    // 이미 여기와 들어 왔다는 것은 데이타가 기존의 데이타와는 다른 새로운 이벤트가 발행 핬다는 것이다.
    // 우리는 기존과 어떤 값이 바뀌었는지를 알아내야 한다
    // 이것을 하기위하여 exclusive OR를 사용하면 어떤 비트의 값이 변경되었는지를 알수 있고 
    // 그리고 지금의 값이 0 혹은 1인지를 확인하여 메세지를 + 하여 그 값을 출력한다. 
    // 해당되는 BIT는 다시 해당 BIT를 & 연산하여 값이 1 혹은 0인지를 판단한다.
    processModBus60() {
        const bitToCheck12_event = [0, 2, 3, 5, 6, 8, 9, 10, 11, 12, 13, 14]; // bits to check for specific conditions
        const eventList = [];
        const alarmList = [];

        // check each bit for specific condition
        let checkRegister = this.Register_12 ^ this.prevRegister_12;
        bitToCheck12_event.forEach((bitIndex) => {
            checkRegister;  // 변경된 값만이 존재 하며 변경된 부분은 1로 설정되어 있다 
            //console.log(`checkRegister:${checkRegister}`)
            const bitValue = checkRegister & (1 << bitIndex);// 12번지에 셋팅되어 있는 비트에 맞는 메세지를 가져온다.   
            //이제 bitValue가 1이면 bitIndex에 해당 하는 비트가 변경되었다는 뜻이다. 
            // 그리고 현재의 값이 O인지 1인지에 따른 메세지를 출력하자 
            //console.log(`bitValue:${bitValue}`)
            const nowBitValue = this.Register_12 & (1 << bitIndex);
            const message = this.getMessage(bitIndex, bitValue, nowBitValue);
            //if (message) console.log(`message :${message.message}`)
            if (message) {
                // add message to event or alarm list based on condition
                //모든 이벤트는 기록한다. 단 alarm은 alarm만 기록한다
                eventList.push(message.message); // if exgist, all event must recored.
                if (message.type === 'alarm') {  //alarm으로 기록된 것만 여기에 기록한다.
                    alarmList.push(message.message);
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
    }

    // method to get message based on bit value and index
    getMessage(bitIndex, bitValue, nowBitValue) {
        //trigger :0 if set to 1
        //trigger :1 if set 0 to 1 
        //trigger :2 if set 1 to 0 

        const messages_12 = {
            0: { type: 'alarm', message: '컨버터 과온', trigger: 0, ON: "경보발생", OFF: "->정상" },/*(0: 정상 , 1: 과온경보)	경보, 이벤트*/
            2: { type: 'event', message: '입력 MC 상태 이벤트', trigger: 0, ON: "->열림", OFF: "->닫힘" },
            3: { type: 'alarm', message: 'Main FUSE Fail(0: 정상 , 1: 이상)	경보, 이벤트', trigger: 0, ON: "->닫힘", OFF: "->열림" },
            5: { type: 'alarm', message: 'Pluse Ground Fault ', trigger: 0, ON: "발생", OFF: "해제" },
            6: { type: 'alarm', message: 'Minus Ground Fault ', trigger: 0, ON: "발생", OFF: "해제" },
            8: { type: 'alarm', message: '축전지 저전압 이상', trigger: 0, ON: "경보", OFF: "해제" },
            9: { type: 'alarm', message: 'CB1 상태 ', trigger: 1, ON: "->닫힘", OFF: " 열림경보" },
            10: { type: 'alarm', message: 'CB2 상태 ', trigger: 1, ON: "->닫힘", OFF: " 열림경보" },
            11: { type: 'alarm', message: 'CB3 상태 ', trigger: 1, ON: "->닫힘", OFF: " 열림경보" },
            12: { type: 'alarm', message: 'Converter_R_GDU Fault ', trigger: 0, ON: "경보발생", OFF: "경보발생해제" },
            13: { type: 'alarm', message: 'Converter_S_GDU Fault ', trigger: 0, ON: "경보발생", OFF: "경보발생해제" },
            14: { type: 'alarm', message: 'Converter_T_GDU Fault ', trigger: 0, ON: "경보발생", OFF: "경보발생해제" }
        };

        const message = messages_12[bitIndex];
        //if (this.prevIndex === -1) //처음이면 prebitr은 고려하지 않는다.
        if (message) {
            if (message.trigger === 0) {
                if (bitValue) { //이 부분이 변경되어야 한다. 
                    // 메세지는 있고, bitValue 가 설정이 되었다면 해당 비트가 셋팅되었다.
                    // 이제 0과 1에 따라 메세지가 발송 되어야 한다. 메세지를 보내야 한다.
                    //console.log(`bitIndex:${bitIndex} bitValue ${bitValue} nowBitValue ${nowBitValue}`)
                    return { type: message.type, message: message.message = nowBitValue ? message.message + message.ON : message.message + message.OFF };
                }
            }
            else if (message.trigger === 1) {
                if (bitValue) {
                    return { type: message.type, message: message.message = nowBitValue ? message.message + message.ON : message.message + message.OFF };
                }
            }
        }
        return null;
    }
}
/**/
class modbusDataArrayClass {
    constructor() {
        this.modBusDataArray = [/* log data received from UPS monitoring system */];
        this.justOnlyEvent = []; // array to store event data
        this.alarmEvent = []; // array to store alarm data
        this.int16Array = new Uint8Array(124);
        this.arrayBuffer = this.int16Array.buffer;
        this.dataView = new DataView(this.arrayBuffer);

        this.dataView.setUint32(0, (new Date()).getTime() / 1000, 1);
        //console.log((new Date()).getTime() / 1000);
        //console.log(this.dataView.getUint32(0, 1));
        this.dataView.setInt16(4 + 24, 0b0000111000000000, 1);
        this.logTime = this.dataView.getUint32(0, 1);
        // 새로운 인스턴스를 만들지만 만들어지 cmdBus는 Array에 들어가지 
        // 않을 수 도 있다. 앞의 데이타와 동일하면 추가 되지 않는다.
        this.cmodBus = new modBusDataClass(this.dataView.buffer, this.justOnlyEvent, this.alarmEvent, this.modBusDataArray);
        //this.cmodBus = new modBusDataClass(this.dataView.buffer);
    }
    addDataArray(dataArrayBuffer,
        event12 = 0b0000111000000000
        , event13 = 0b0000000000000000
        , event14 = 0b0000000000000000
        , event15 = 0b0000000000000000
    ) {
        this.int16Array = new Uint8Array(dataArrayBuffer);
        this.arrayBuffer = this.int16Array.buffer;
        this.dataView = new DataView(this.arrayBuffer);
        this.dataView.setUint32(0, (new Date()).getTime() / 1000, 1);
        this.dataView.setInt16(4 + 24, event12, 1);
        this.logTime = this.dataView.getUint32(0, 1);

        this.cmodBus = new modBusDataClass(this.dataView.buffer, this.justOnlyEvent, this.alarmEvent, this.modBusDataArray);
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
        let logHeading = document.createElement("h1");
        logHeading.innerHTML = heading;
        document.getElementById('HtmlLogView').appendChild(logHeading);
        //document.body.appendChild(logHeading);
        let table = document.createElement("table");
        table.style.border = "1px solid black";
        //   for (var i = 0; i < lines.length; i++) {
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
        document.getElementById('HtmlLogView').appendChild(table);
        //document.getElementById('HtmlLogView').style.display= "none";
    }
}
async function testClassCode() {
    let modData = new modbusDataArrayClass();
    let int16Array = new Uint8Array(124);
    modData.addDataArray(int16Array, 0b0000111000000101);
    await new Promise(resolve => setTimeout(resolve, 1000));
    modData.addDataArray(int16Array, 0b0000111000000100);
    await new Promise(resolve => setTimeout(resolve, 1000));
    modData.addDataArray(int16Array, 0b0000111000000000);
    modData.printEvent(modData.alarmEvent.reverse(), "Alarm Log");
    modData.printEvent(modData.justOnlyEvent.reverse(), "Event Log");
    //modData.printAlarm("AlarmLog");

}
testClassCode();

// modDa.justOnlyEvent.forEach(ev => {
//     console.log(ev.events)
// });


// loop through log data and process modBus60 register for each entry
//modBusDataArray = [/* log data received from UPS monitoring system */];
/*
const int16Array = new Uint8Array(124);
const arrayBuffer = int16Array.buffer;
const dataView = new DataView(arrayBuffer);

dataView.setUint32(0, (new Date()).getTime(), 1);
dataView.setInt16(4 + 24, 0b0000111000000000, 1);
logTime = dataView.getUint32(0, 1);
cmodBus1 = new modBusDataClass(dataView.buffer);

dataView.setUint32(0, (new Date()).getTime(), 1);
dataView.setInt16(4 + 24, 0b0000110000000000, 1);
logTime = dataView.getUint32(0, 1);
cmodBus2 = new modBusDataClass(dataView.buffer);

dataView.setUint32(0, (new Date()).getTime(), 1);
dataView.setInt16(4 + 24, 0b0000111000000000, 1);
logTime = dataView.getUint32(0, 1);
cmodBus3 = new modBusDataClass(dataView.buffer);

console.log('justOnlyEvent', justOnlyEvent);
console.log('alarmEvent', alarmEvent);
justOnlyEvent.forEach(ev => {
    console.log(ev.events)
});
alarmEvent.forEach(er => {
    console.log(er.alarms)
});
*/

// modBusDataArray.push(new modBusDataClass(dataView.buffer));
// modBusDataArray.forEach((data) => {
//     const modBusData = data;//new modBusDataClass(data);
//     modBusData.processModBus60();
// });
        //console.log("modbusarray " + this.prveIndenx)
        // if (modBusDataArray.length > 0) {
        //     let length = modBusDataArray.length
        //     this.Register_12 = modBusDataArray[length - 1].modBus60[12];
        //     this.Register_13 = modBusDataArray[length - 1].modBus60[13];
        //     this.Register_14 = modBusDataArray[length - 1].modBus60[14];
        //     this.Register_15 = modBusDataArray[length - 1].modBus60[15];
        //     this.stringfyModBus60 = modBusDataArray[length - 1].stringfyModBus60;
        // }
        //this.modBusDataArray = [];/* log data received from UPS monitoring system */
        //this.justOnlyEvent = []; // array to store event data
        //this.alarmEvent = []; // array to store alarm data
        //this.modBusDataArray = [];/* log data received from UPS monitoring system */
        //this.justOnlyEvent = []; // array to store event data
        //this.alarmEvent = []; // array to store alarm data