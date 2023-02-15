"use strict"
//const modBusDataArray = [/* log data received from UPS monitoring system */];
class modBusDataClass {
    constructor(dataArrayBuffer,justOnlyEvent,alarmEvent,modBusDataArray  ) {

        this.justOnlyEvent=justOnlyEvent;
        this.alarmEvent=alarmEvent  ;
        this.modBusDataArray=modBusDataArray ;

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
                console.log("log changed " + this.prevRegister_12);
                this.processModBus60();
                modBusDataArray.push(this);
            }
        }
        else {
            this.prevIndex = - 1;
            this.processModBus60();
            modBusDataArray.push(this);
        }
    }
    // method to process the modBus60 register and get the changes
    processModBus60() {
        const bitToCheck12_event = [0, 2, 3, 5, 6, 8, 9, 10, 11, 12, 13, 14]; // bits to check for specific conditions
        const eventList = [];
        const alarmList = [];

        // check each bit for specific condition
        bitToCheck12_event.forEach((bitIndex) => {
            const bitValue = this.Register_12 & (1 << bitIndex);
            const prevbitValue = this.prevRegister_12 & (1 << bitIndex);
            const message = this.getMessage(bitIndex, bitValue, prevbitValue);

            if (message) {
                // add message to event or alarm list based on condition
                eventList.push(message.message); // if exgist, all event must recored.
                if (message.type === 'alarm') {  //alarm으로 기록된 것만 여기에 기록한다.
                    alarmList.push(message.message);
                }
            }
        });
        // add log time and event/alarm list to appropriate array only if there are changes
        if (eventList.length > 0 || alarmList.length > 0) {
            const newModBus60 = JSON.stringify(this.modBus60); // get the stringified modBus60
            console.log(newModBus60);
            if (this.stringfyModBus60 === null || this.stringfyModBus60 !== newModBus60) {
                // save the changes only if there are no previous data or the current data is different from the previous data
                if (eventList.length > 0) {
                    this.justOnlyEvent.push({ logTime: this.logTime, events: eventList });
                }
                if (alarmList.length > 0) {
                    this.alarmEvent.push({ logTime: this.logTime, alarms: alarmList });
                }
                this.stringfyModBus60 = newModBus60;
            }
        }
    }

    // method to get message based on bit value and index
    getMessage(bitIndex, bitValue, prevbitValue) {
        //trigger :0 if set to 1
        //trigger :1 if set 0 to 1 
        //trigger :2 if set 1 to 0 

        const messages_12 = {
            0: { type: 'alarm', message: '컨버터 과온(0: 정상 , 1: 과온경보)	경보, 이벤트', trigger: 0 },
            2: { type: 'event', message: '입력 MC 상태(0:열림, 1:닫힘)	이벤트', trigger: 0 },
            3: { type: 'alarm', message: 'Main FUSE Fail(0: 정상 , 1: 이상)	경보, 이벤트', trigger: 0 },
            5: { type: 'alarm', message: 'Pluse Ground Fault	경보, 이벤트', trigger: 0 },
            6: { type: 'alarm', message: 'Minus Ground Fault	경보, 이벤트', trigger: 0 },
            8: { type: 'alarm', message: '축전지 저전압 이상(0: 정상 , 1: 이상)	경보, 이벤트', trigger: 0 },
            9: { type: 'alarm', message: 'CB1 상태(0:열림, 1:닫힘)	경보:열림, 이벤트:열림,닫힘', trigger: 2 },
            10: { type: 'alarm', message: 'CB2 상태(0:열림, 1:닫힘)	경보:열림, 이벤트:열림,닫힘', trigger: 2 },
            11: { type: 'alarm', message: 'CB3 상태(0:열림, 1:닫힘)	경보:열림, 이벤트:열림,닫힘', trigger: 2 },
            12: { type: 'alarm', message: 'Converter_R_GDU Fault(0: 정상 , 1: 이상)	경보, 이벤트', trigger: 0 },
            13: { type: 'alarm', message: 'Converter_S_GDU Fault(0: 정상 , 1: 이상)	경보, 이벤트', trigger: 0 },
            14: { type: 'alarm', message: 'Converter_T_GDU Fault(0: 정상 , 1: 이상)	경보, 이벤트', trigger: 0 },
        };

        const message = messages_12[bitIndex];
        console.log(`bitIndex:${bitIndex} bitValue ${bitValue} prebit ${prevbitValue}`)
        //if (this.prevIndex === -1) //처음이면 prebitr은 고려하지 않는다.
        if (message) {
            if (message.trigger === 0) {
                if (message && bitValue) {
                    return { type: message.type, message: message.message };
                }
            }
            else if (message.trigger === 1) {
                if (message && bitValue) {
                    return { type: message.type, message: message.message };
                }
            }
            else if (message.trigger === 2) { //이 경우는 1에서는 경보를 내지 않는다. 0으로 가야 경보를 낸다.
                if (message && bitValue === 0) {
                    console.log(`여기에 왔어요.. ${bitValue} ${bitValue === 0}`)
                    return { type: message.type, message: message.message };
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

        this.dataView.setUint32(0, (new Date()).getTime(), 1);
        this.dataView.setInt16(4 + 24, 0b0000111000000000, 1);
        this.logTime = this.dataView.getUint32(0, 1);
        // 새로운 인스턴스를 만들지만 만들어지 cmdBus는 Array에 들어가지 
        // 않을 수 도 있다. 앞의 데이타와 동일하면 추가 되지 않는다.
        this.cmodBus = new modBusDataClass(this.dataView.buffer,this.justOnlyEvent,this.alarmEvent ,this.modBusDataArray );
        //this.cmodBus = new modBusDataClass(this.dataView.buffer);
    }
    addDataArray(dataArrayBuffer,
            event12=0b0000111000000000
            ,event13=0b0000000000000000
            ,event14=0b0000000000000000
            ,event15=0b0000000000000000
        ){
        this.int16Array = new Uint8Array( dataArrayBuffer);
        this.arrayBuffer = this.int16Array.buffer;
        this.dataView = new DataView(this.arrayBuffer);
        this.dataView.setUint32(0, (new Date()).getTime(), 1);
        this.dataView.setInt16(4 + 24, event12, 1);
        this.logTime = this.dataView.getUint32(0, 1);

        this.cmodBus = new modBusDataClass(this.dataView.buffer,this.justOnlyEvent,this.alarmEvent,this.modBusDataArray  );
    }
    printEvent(){
       console.log(this.justOnlyEvent)
       this.justOnlyEvent.forEach(ev => {
        console.log(ev.events)
       });
    }
    printAlarm(){
       console.log(this.justOnlyEvent)
        this.alarmEvent.forEach(er => {
            console.log(er.alarms)
        });
    }
}
let modData = new modbusDataArrayClass();
let int16Array = new Uint8Array(124);
modData.addDataArray(int16Array,0b0000111000000001);
modData.addDataArray(int16Array,0b0000111000000100);
modData.printEvent();
modData.printAlarm();

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