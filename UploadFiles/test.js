"use strict"
const warningImage = new Image();
warningImage.src = 'alert.svg';
warningImage.width = 50;
warningImage.height = 50;

//const beepSound = 'bell-ringing.mp3';
//const warningSound = new Audio(beepSound);
//warningSound.loop = true;

const warningSound = document.getElementById('warning-sound');
//const unmuteButton = document.getElementById('unmute-button');
let alarmStatus = false;
let flashInterval = null; // Declare a variable to store the interval ID

// Add an event listener that listens for changes to the alarm status
// document.addEventListener('alarmStatusChanged', function (event) {
//     console.log('event.detail ${event.detail}' + event.detail)
//     if (event.detail === true) {
//         // If the alarm status is true, play the sound and flash the image
//         if (alarmStatus == false) {
//             warningSound.play();
//             alarmStatus = true;
//             document.body.appendChild(warningImage);
//             if (flashInterval !== null) {
//                 clearInterval(flashInterval); // Clear the previous interval
//             }
//             flashInterval = setInterval(() => {
//                 const img = document.querySelector('img');
//                 if (img) {
//                     img.style.visibility = (img.style.visibility === 'hidden') ? 'visible' : 'hidden';
//                 }
//             }, 500);
//         }
//     } else {
//         // If the alarm status is false, stop the sound and remove the image
//         if (alarmStatus === true) {
//             alarmStatus =false;
//             warningSound.muted = false;
//             warningSound.currentTime = 0;
//             document.body.removeChild(warningImage);
//         }
//     }
// });

//Function to set the alarm status and trigger the event listener
function fireAlarmStatus(status) {
    console.log(`fireAlarmStatus(${status})`)
    //alarmStatus = status;
    let alarmStatusEvent = new CustomEvent('alarmStatusChanged', { detail: status });
    document.dispatchEvent(alarmStatusEvent);
}
// function beep() {
//     try{
//         warningSound.play();
//     }catch(error){
//         console.log('An error occurred while playing the warning sound: ', error);
//     }
// };
// function showWarning() {
//     console.log(alarmStatus)
//     if (!alarmStatus) {
//         document.body.appendChild(warningImage);
//         beep();
//         alarmStatus = true;
//         let flashInterval = setInterval(() => {
//             const img = document.querySelector('img');
//             if (img) {
//                 img.style.visibility = (img.style.visibility === 'hidden') ? 'visible' : 'hidden';
//             }
//         }, 500);
//     }
// }

// function stopWarning() {
//     if (alarmStatus) {
//         document.querySelector('img').remove();
//         try{
//             warningSound.pause();
//         }catch(error){
//             console.log('An error occurred while playing the warning sound: ', error);
//         }
//         warningSound.currentTime = 0;
//         alarmStatus = false;
//     }
// }

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
    addData(dataArrayBuffer) {
        const int16Array = new Uint8Array(dataArrayBuffer);
        const arrayBuffer = int16Array.buffer;
        const dataView = new DataView(arrayBuffer);
        this.logTime = dataView.getUint32(0, 1);
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
                console.log("log changed " + this.prevRegister_12);
                this.processModBus60();
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
            14: { type: 'alarm', message: '', trigger: 0, ON: "복전", OFF: "" }
        };
        //const bitToCheck_15_event = [1,9,10];
        const messages_15 = {
            1: { type: 'alarm', message: '과부하 정지 ', trigger: 0, ON: "발생", OFF: "후 회복" },
            9: { type: 'alarm', message: '과부하 ', trigger: 0, ON: "시간제한이상발생 ", OFF: "시간제한이상발생후 회복 " },
            10: { type: 'alarm', message: '충전기 ', trigger: 0, ON: "운전중", OFF: "정지상태" }
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
                    return { type: message.type = nowBitValue ? 'event' : message.type, message: message.message = nowBitValue ? message.message + message.ON : message.message + message.OFF };
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
        this.int16Array = new Uint8Array(124);
        this.arrayBuffer = this.int16Array.buffer;
        this.dataView = new DataView(this.arrayBuffer);

        this.dataView.setUint32(0, (new Date()).getTime() / 1000, 1);
        //console.log((new Date()).getTime() / 1000);
        //console.log(this.dataView.getUint32(0, 1));
        this.dataView.setInt16(4 + 24, 0b0000111000000000, 1);
        this.dataView.setInt16(4 + 26, 0b0000000000000011, 1);
        this.dataView.setInt16(4 + 28, 0b0000000000000000, 1);
        this.dataView.setInt16(4 + 30, 0b0000000000000000, 1);
        this.logTime = this.dataView.getUint32(0, 1);
        // 새로운 인스턴스를 만들지만 만들어지 cmdBus는 Array에 들어가지 
        // 않을 수 도 있다. 앞의 데이타와 동일하면 추가 되지 않는다.
        this.cmodBus = new modBusDataClass(this.dataView.buffer, this.justOnlyEvent, this.alarmEvent, this.modBusDataArray);
    };
    addEventListener(eventname, callback) {
        document.addEventListener(eventname, callback);
    }
    setAlarmStatus(status) {
        //alarmStatus = status;
        let alarmStatusEvent = new CustomEvent('alarmStatusChanged', { detail: status });
        document.dispatchEvent(alarmStatusEvent);
        // if (status === true) {
        //     // If the alarm status is true, play the sound and flash the image
        //     warningSound.play();
        //     console.log('warningSound.play();')
        //     document.body.appendChild(warningImage);
        //     let flashInterval = setInterval(() => {
        //         const img = document.querySelector('img');
        //         if (img) {
        //             img.style.visibility = (img.style.visibility === 'hidden') ? 'visible' : 'hidden';
        //         }
        //     }, 500);
        // } else {
        //     // If the alarm status is false, stop the sound and remove the image
        //     warningSound.muted = false;
        //     warningSound.currentTime = 0;
        //     //document.body.removeChild(warningImage);
        // }

    };
    addDataArray(dataArrayBuffer,
        event12 = 0b0000111000000000
        , event13 = 0b0000000000000011
        , event14 = 0b0000000000000000
        , event15 = 0b0000000000000000
    ) {
        this.int16Array = new Uint8Array(dataArrayBuffer);
        this.arrayBuffer = this.int16Array.buffer;
        this.dataView = new DataView(this.arrayBuffer);
        this.dataView.setUint32(0, (new Date()).getTime() / 1000, 1);
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
        event14_alarm = (event14_alarm & (1 << 14)) ? event14_alarm & ~(1 << 14) : event14_alarm | (1 << 14);
        // || alarmBitMask_14 & event14_alarm
        if (alarmBitMask_12 & event12_alarm || alarmBitMask_13 & event13_alarm || alarmBitMask_14 & event14_alarm) {
            //showWarning();
            console.log('setAlarmStatus(true);')
            //this.setAlarmStatus(true); // Turn on the alarm
        }
        else {
            //stopWarning();
            console.log('setAlarmStatus(false);')
            //this.setAlarmStatus(false); // Turn on the alarm
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
        let logHeading = document.createElement("h1");
        logHeading.innerHTML = heading;
        document.getElementById('HtmlLogView').appendChild(logHeading);
        let table = document.createElement("table");
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
        document.getElementById('HtmlLogView').appendChild(table);
        //document.getElementById('HtmlLogView').style.display= "none";
    }
}


async function testClassCode() {
    //modData.beep();
    let modData = new modbusDataArrayClass();
    modData.addEventListener('alarmStatusChanged', function (event) {
    console.log('event.detail ${event.detail}' + event.detail)
    if (event.detail === true) {
        // If the alarm status is true, play the sound and flash the image
        if (alarmStatus == false) {
            warningSound.play();
            warningSound.loop = true;
            alarmStatus =true;
            document.body.appendChild(warningImage);
            if (flashInterval !== null) {
              clearInterval(flashInterval); // Clear the previous interval
             }
            flashInterval = setInterval(() => {
                const img = document.querySelector('img');
                if (img) {
                    img.style.visibility = (img.style.visibility === 'hidden') ? 'visible' : 'hidden';
                }
            }, 500);
        }
    } else {
        // If the alarm status is false, stop the sound and remove the image
        if (alarmStatus === true) {
            alarmStatus =false;
            warningSound.loop = false;
            warningSound.muted = false;
            warningSound.currentTime = 0;
            document.body.removeChild(warningImage);
        }
    }
});
    let int16Array = new Uint8Array(124);
    //modData.addDataArray(int16Array, 0b0000111000000000);
    //await new Promise(resolve => setTimeout(resolve, 1000));
    //modData.addDataArray(int16Array, 0b0000111000000100);
    //await new Promise(resolve => setTimeout(resolve, 1000));
    //modData.addDataArray(int16Array, 0b0000111000000000);
    console.log("First")
    modData.addDataArray(int16Array, 0b0000111000000000, 0b0000000000000011, 0b0100000000000000, 0b0000000000000000);//
    //modData.addDataArray(int16Array, 0b0000111000000000, 0b0000000000000011, 0b0100000000000000, 0b0000000000000000);//
    //modData.showWarning()
    //modData.addDataArray(int16Array, 0b0000110000000000, 0b0000000000000011, 0b0000000000000000, 0b0000000000000000);//
    //modData.addDataArray(int16Array, 0b0000110000000000, 0b0000000000000011, 0b0000000000000000, 0b0000000000000000);//
    //modData.addDataArray(int16Array, 0b0000111000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000);//
    //modData.addDataArray(int16Array, 0b0000111000000001, 0b0000000000000001, 0b0000000000000000, 0b0000000000000000);//
    modData.printEvent(modData.alarmEvent.reverse(), "Alarm Log");
    modData.printEvent(modData.justOnlyEvent.reverse(), "Event Log");
    //modData.printAlarm("AlarmLog");

}

// const showButton = document.querySelector('#showButton');
// const stopButton = document.querySelector('#stopButton');

document.getElementById('showButton').addEventListener('click', (e) => {
    fireAlarmStatus(true);

});
document.getElementById('stopButton').addEventListener('click', (e) => {
    fireAlarmStatus(false);

});
window.onload = function () {
    console.log("onLoad");
    testClassCode();
};



// function showWarning_() {
//     showWarning();
// }
// function stopWarning_() {
//     stopWarning();
// }
// attach the showWarning and stopWarning functions to button click events


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