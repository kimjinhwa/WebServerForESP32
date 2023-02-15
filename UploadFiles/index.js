var receiveData="";
var modBusDataArray=[];
class modBusDataClass {
  constructor(dataArrayBuffer) {
    const int16Array = new Uint8Array(dataArrayBuffer);
    const arrayBuffer = int16Array.buffer;
    const dataView = new DataView(arrayBuffer);
    this.logTime = dataView.getUint32(0,1);
    this.modBus60 = [];
    for (let i = 0; i < 60; i++) {
      this.modBus60[i] = dataView.getInt16(4 + (2 * i),1);
    }
    //console.log(this.modBus60);
  }
}
var modbus_registor = {
    "time": 0,
    "value":[
      0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0
    ],
};
modbus_address =
{
0:"제조 년",
1:"제조 월",
2:"제조 일",
3:"챠져 용량(kva)",
4:"입력 상(3)",
5:"입력 전압",
7:"출력 정격전압",
8:"제조사 명", /* (4: 대농산업전기)*/
9:"밧데리 셀",
12:"Converter state",
13:"Inverter state",
14:"Converter Operation Fault",
15:"Inverter Operation Fault",
20:'입력 R상 전압',
21:'입력 S상 전압',
22:'입력 T상 전압',
23:'입력 R상 전류',
24:'입력 S상 전류',
25:'입력 T상 전류',
26:'입력 주파수',
27:'충전기 출력 전압 ',
28:'SID출력 전압 계측값',
29:'축전지 전압 계측값',
30:'축전지 전류 계측값',
31:'직류출력 전류 ',
32:'부하율',
}
// Define a structure to store the bit information and its corresponding message
const EventBitmask_12= 0b0111111101101101 ; 
const AlarmBitmask_12= 0b0111111101101001 ; 
const ConStBitInfo_12 = [
{bit:0,message: "컨버터 과온"},/*(0: 정상 , 1: 과온경보)	경보, 이벤트*/
{bit:2,message: "	입력 MC 상태"},/*(0:열림, 1:닫힘)	이벤트만*/
{bit:3,message: "	Main FUSE Fail"},/*(0: 정상 , 1: 이상)	경보, 이벤트*/
{bit:5,message: "	Pluse Ground Fault"},/*	경보, 이벤트*/
{bit:6,message: "	Minus Ground Fault"},/*	경보, 이벤트*/
{bit:8,message:  "  XX	축전지 저전압 이상"},/*(0: 정상 , 1: 이상)	경보, 이벤트*/
{bit:9,message: "CB1 상태"},/*(0:열림, 1:닫힘)	경보:열림만, 이벤트:열림,닫힘*/
{bit:10,message: "	CB2 상태"},/*(0:열림, 1:닫힘)	경보:열림만, 이벤트:열림,닫힘*/
{bit:11,message: "	CB3 상태"},/*(0:열림, 1:닫힘)	경보:열림만, 이벤트:열림,닫힘*/
{bit:12,message: "	Converter_R_GDU Fault"},/*(0: 정상 , 1: 이상)	경보, 이벤트*/
{bit:13,message: "	Converter_S_GDU Fault"},/*(0: 정상 , 1: 이상)	경보, 이벤트*/
{bit:14,message: "	Converter_T_GDU Fault"},/*(0: 정상 , 1: 이상)	경보, 이벤트*/
];
const EventBitmask_13= 0b0000000101100111 ; 
const AlarmBitmask_13= 0b0000000101100111 ; 
//열림이벤트만 검사 하는 방법은 한번 더 검사를 하는 것이다.
//이전 이벤트와 비교해서, 열림인지 혹은 닫힘 인지를 검사한다.
const InvStBitInfo_13 =[
    {bit:0,message:"	CB4 상태(0:열림, 1:닫힘)	"},/*경보:열림만, 이벤트:열림,닫힘 */
		{bit:1,message:"	CB5 상태(0:열림, 1:닫힘)	"},/*경보:열림만, 이벤트:열림,닫힘 */
		{bit:2,message:"	FAN Fault 이상,/* ( 0: 정상, 1: 이상)	경보, 이벤트"},/* */
		{bit:5,message:"	입력 과전류 이상"},/* ( 0: 정상, 1: 이상)	경보, 이벤트*/
		{bit:6,message:"	배터리 과전압 이상"},/*( 0: 정상, 1: 이상)	경보, 이벤트 */
		{bit:8,message:"	배터리 저전압 이상"}/* ( 0: 정상, 1: 이상)	경보, 이벤트*/
];

const EventBitmask_14= 0x00;
const AlarmBitmask_14= 0b0111111111111110 ;
const ConOpFaultBitInfo_14 =[
  {bit:1,message:	"충전기 전류제한이상"},/*(0: 정상, 1: 이상)*/
  {bit:2,message:	"배터리 충전전류제한이상"},/*(0: 정상, 1: 이상)*/
  {bit:3,message:	"배터리 과전압제한이상"},/*(0 : 정상, 1 : 이상)*/
  {bit:4,message:	"배터리 저전압제한이상"},/*(0 : 정상, 1 : 이상)*/
  {bit:5,message:	"입력 상회전 "},/*이상( 0: 정상, 1: 이상)*/
  {bit:6,message:	"입력 저전압 "},/*이상( 0: 정상, 1: 이상)*/
  {bit:7,message:	"입력 과전압 "},/*이상( 0: 정상, 1: 이상)*/
  {bit:8,message:	"전력 모듈이상"},/*(0:정상, 1: 이상)*/
  {bit:9,message:	"입력 주파수 "},/*이상( 0: 정상, 1: 이상)*/
  {bit:10,message:"	입력 "},/*비동기 이상( 0: 정상, 1: 이상)*/
  {bit:11,message:"	정전"},/*( 0: 정상, 1: 이상)*/
  {bit:12,message:"	입력 "},/*MC 이상( 0: 정상, 1: 이상)*/
  {bit:13,message:"	결상"},/*( 0: 정상, 1: 이상)*/
  {bit:14,message:"	복전"}/*(1: 복전)*/
];
const EventBitmask_15= 0x00;
const AlarmBitmask_15= 0b0000011000000010 ; 
const InvOpFaultBitInfo15 =[
    {bit:1,message:"	과부하 정지"},/*(0:정상, 1: 이상)*/
    {bit:9,message:"	과부하 시간제한이상"},/*(0:정상, 1: 이상)*/
    {bit:10,message:"	충전기 운전/정지상태"}/*(0:정지, 1: 운전중)*/
];

let obj = {};
//let converterState = 0;
//val inputVol =document.getElementById('inputVol');
Object.defineProperty(obj, 'inputVoltage', {
  get: function() {
    return inputVoltage;
  },
  set: function(value) {
  inputVoltage= value;
  inputVol.innerHTML = value;
  }
});
//obj.inputVoltage=280;
Object.defineProperty(obj, 'converterState', {
  get: function() {
    return converterState;
  },
  set: function(value) {
    converterState = value;
    // execute the for loop when the value of converterState is changed
    for (let i = 0; i < ConStBitInfo_12.length; i++) {
      if (converterState & (1 << ConStBitInfo_12[i].bit)) {
        console.log(ConStBitInfo_12[i].message);
      }
    }
  }
});
Object.defineProperty(obj, 'inverterState', {
  get: function() {
    return this._inverterState;
  },
  set: function(value) {
    this._inverterState = value;
    // execute the for loop when the value of inverterState is changed
    for (let i = 0; i < ConStBitInfo_12.length; i++) {
      if (this._inverterState & (1 << ConStBitInfo_12[i].bit)) {
        console.log(ConStBitInfo_12[i].message);
      }
    }
  }
});

console.log("test code");
obj.converterState = (1 << 5) | (1 << 6);
obj.converterState = (1 << 8) | (1 << 9);
var alarm_ad12_set= [
  '충전기 과열 이상',
  '',
  '입력 M/C 닫힘',
  '충전기 퓨즈 이상',
  '밧데리 (+) 지락 발생',
  '밧데리 (-) 지락 발생',
  '릴레이 접점 리셋',
  '긴급 이상',
  'CB1 차단기 닫힘',
  'CB3 차단기 열림',
  '',
  '충전기 R상 GDU 이상',
  '충전기 S상 GDU 이상',
  '충전기 T상 GDU 이상',
  '',
  ''
];  
var alarm_ad12_clr= [
  '',
  '',
  '입력 M/C 열림',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  'CB1 차단기 열림',
  'CB3 차단기 닫힘',
  '',
  '',
  ''
];



var alarm_ad13_set= [
  'CB2 차단기 닫힘',
  'CB4 차단기 닫힘',
  '팬1 이상 발생',
  '',
  '',
  '충전기 과전류 이상',
  '축전지 과전압 이상',
  '',
  '축전지 저전압 이상',
  '',
  '',
  '',
  '',
  '',
  '옵셋 체크 이상',
  ''
]


var alarm_ad13_clr= [
  'CB2 차단기 열림',
  'CB4 차단기 열림',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  '',
  ''
]




var alarm_ad14_set= [
  '',
  '충전기 전류 제한 이상',
  '축전지 전류 제한 이상',
  '축전지 과전압 제한 이상',
  '축전지 저전압 제한 이상',
  '입력 상회전 이상',
  '입력 저전압 이상',
  '입력 과전압 이상',
  '',
  '입력 주파수 이상',
  '입력비동기 이상',
  '정전',
  '입력 M/C 이상',
  '결상',
  '복전',
  ''
]

var alarm_ad15_set= [
'',
'',
'',
'',
'',
'',
'',
'',
'',
'출력 과부하',
'',
'',
'',
'',
'',
'',
]



var alarm_ad16_set= [
'부동충전 모드 설정',
'균등충전 모드 설정',
'충전기 운전',
'충전기 자동운전',
'충전기 정지',
'이상발생 충전기 정지',
'',
'',
'',
'',
'',
'',
'바이패스로 절환',
'이상발생-바이패스로 절환',
'자동운전 시작',
''
]


var width = 850,
  height = 300;
var bat_w = 70,
  bat_h = 45;

var draw = SVG().addTo('#drawing').size(width, height);
var background = draw.rect(width, height).fill('#E3E8E6');

var start_x = 10,
  start_y = 100,
  powerLineWidth = 15,
  PL_1_length = 60,
  PL_2_length = 40,
  PL_3_length = 40;

var PL_1 = draw.use(
  draw
    .symbol()
    .add(
      draw
        .rect(PL_1_length, powerLineWidth)
        .stroke({ width: 1, color: 'black' })
        .move(start_x, start_y)
    )
);
PL_1.fill('none');

start_x = start_x + PL_1_length;

////
// CB
var sbSymbol = draw.symbol();
start_y += 15;
sbSymbol.add(
  draw
    .path('M+20' + ' ' + start_y + ' v-15h-15v-12h45v12h-15v15z')
    .stroke({ width: 1, color: 'black' })
);
var CB_1 = draw
  .use(sbSymbol)
  .move(start_x - 20, 0)
  .fill('none');

start_y -= powerLineWidth;

start_x += powerLineWidth;

drawTime = draw.text('00:00:00');

var PL_2 = draw.use(
  draw
    .symbol()
    .add(
      draw
        .rect(PL_2_length, powerLineWidth)
        .stroke({ width: 1, color: 'black' })
        .move(start_x, start_y)
    )
);
start_x = start_x + PL_2_length;

var converTer = draw.symbol();
converTer.add(
  draw
    .rect(bat_w, bat_h)
    .stroke({ width: 1, color: 'black' })
    .move(start_x, start_y + powerLineWidth / 2 - bat_h / 2)
);

converTer.add(
  draw
    .line(
      start_x,
      start_y + powerLineWidth / 2 + bat_h / 2,
      start_x + bat_w,
      start_y + powerLineWidth / 2 - bat_h / 2
    )
    .stroke({ width: 1, color: 'black' })
);

converTer.add(
  draw
    .text('~')
    .stroke({ width: 1, color: 'black' })
    .move(start_x + bat_w / 5, start_y - bat_w / 4)
    .scale(2)
);
converTer.add(
  draw
    .text('_')
    .stroke({ width: 1, color: 'black' })
    .move(start_x + bat_w / 1.6, start_y - bat_w / 5)
    .scale(2)
);
var batteryConverter = draw.use(converTer).fill('red');

//end bat draw
start_x += bat_w;
var PL_3 = draw.use(
  draw
    .symbol()
    .add(
      draw
        .rect(PL_3_length, powerLineWidth)
        .stroke({ width: 1, color: 'black' })
        .move(start_x, start_y)
    )
);
//var PL_3 = ;
start_x = start_x + PL_3_length;

var CB_2 = draw
  .use(sbSymbol)
  .move(start_x - 20, 0)
  .fill('none');
start_x = start_x + powerLineWidth;

var pathString = '';
var inc_w = 300,
  inc_h = 40;
pathString = 'M' + start_x + ' ' + start_y;
pathString += ' H' + inc_w;
pathString += ' V' + inc_h;
inc_w += 40;
pathString += ' H' + inc_w;
inc_h += powerLineWidth;
pathString += ' V' + inc_h;
inc_w += -25;
pathString += ' H' + inc_w;
inc_h += 180;
pathString += ' V' + inc_h;
inc_w += 80;
pathString += ' H' + inc_w;
inc_h += powerLineWidth;
pathString += ' V' + inc_h;
inc_w -= 95;
pathString += ' H' + inc_w;
inc_h -= 135;
pathString += ' V' + inc_h;
inc_w -= 50;
pathString += ' H' + inc_w;
pathString += 'z';

var PL_4 = draw
  .use(
    draw
      .symbol()
      .add(draw.path(pathString).stroke({ width: 1, color: 'black' }))
  )
  .fill('none');

var MC_draw = draw.symbol();
MC_draw.add(
  draw.circle(15).fill('cyan').stroke({ width: 1, color: 'black' }).move(2.5, 0)
);
MC_draw.add(draw.rect(55, 15).stroke({ width: 1, color: 'black' }).move(20, 0));
start_x += 90;
start_y += -60;
var MC_1 = draw.use(MC_draw).move(start_x, start_y).fill('none');
start_x += 75;
var MC_2 = draw.use(MC_draw).move(start_x, start_y).fill('none');
start_x += 75;
var MC_3 = draw.use(MC_draw).move(start_x, start_y).fill('none');
start_x += 75;
var MC_4 = draw.use(MC_draw).move(start_x, start_y).fill('none');
start_x += 75;
var MC_5 = draw
  .circle(15)
  .stroke({ width: 1, color: 'black' })
  .move(192.5 + 75 * 6, 100 - 60);
MC_5.fill('cyan');

//SID Draw
var sid_draw = draw.symbol();
sid_draw.add(
  draw.rect(30, 30).stroke({ width: 1, color: 'black' }).move(340, 73)
);
sid_draw.add(
  draw.path('M350 100 v-20l10 10zv-10 ').stroke({ width: 1, color: 'black' })
);
sid_draw.add(
  draw
    .path('M350 90 h-5m15 0 h5 m-5 -10 v20')
    .fill('none')
    .stroke({ width: 1, color: 'black' })
);

var SID_1 = draw.use(sid_draw).move(-5, +20).fill('none');
var SID_2 = draw.use(sid_draw).move(70, +20).fill('none');
var SID_3 = draw.use(sid_draw).move(145, +20).fill('none');
var SID_4 = draw
  .use(sid_draw)
  .move(145 + 75, +20)
  .fill('none');

var SID_5 = draw
  .use(sid_draw)
  .move(145 + 75 + 75, +20)
  .fill('none');
var PL_5 = draw
  .use(
    draw
      .symbol()
      .add(
        draw.rect(20, 15).stroke({ width: 1, color: 'black' }).move(315, 100)
      )
  )
  .fill('none');
var PL_6 = draw
  .path('M665 100h40v-45h-45v-15h60v60h30v15h-30v60h30v15h-45v-75h-40z')
  .fill('none')
  .stroke({ width: 1, color: 'black' });

var PL_relay = draw.symbol();
PL_relay.add(
  draw
    .path('M365 100 h15v-45h15v45h15v15h-45z')
    .stroke({ width: 1, color: 'black' })
);

var PRL_1 = draw.use(PL_relay).fill('none');
var PRL_2 = draw.use(PL_relay).move(75, 0).fill('none');
var PRL_3 = draw
  .use(PL_relay)
  .move(75 + 75, 0)
  .fill('none');
var PRL_4 = draw
  .use(PL_relay)
  .move(75 + 75 + 75, 0)
  .fill('none');

var CB_4 = draw.use(sbSymbol).move(375, 135).fill('none');
var PL_BAT = draw
  .rect(60, 15)
  .fill('none')
  .stroke({ width: 1, color: 'black' })
  .move(410, 235);
var batSymbol = draw.symbol();
batSymbol.add(
  draw
    //.path('M100 100 h15v-45h15v45h15v15h-45z')
    .rect(70, 50)
    .stroke({ width: 2, color: 'black' })
);
batSymbol.add(
  draw
    //.path('M100 100 h15v-45h15v45h15v15h-45z')
    .line(10, 20, 60, 20)
    .stroke({ width: 8, color: 'black' })
);
batSymbol.add(
  draw
    //.path('M100 100 h15v-45h15v45h15v15h-45z')
    .line(20, 35, 50, 35)
    .stroke({ width: 4, color: 'black' })
);
var batteryBank = draw.use(batSymbol).move(470, 215).fill('none');

var CB_5 = draw.use(sbSymbol).move(730, 0).fill('none');
var PL_7 = draw
  .rect(60, 15)
  .fill('none')
  .stroke({ width: 1, color: 'black' })
  .move(765, 100);

var CB_6 = draw.use(sbSymbol).move(730, 75).fill('none');
var PL_8 = draw
  .rect(60, 15)
  .stroke({ width: 1, color: 'black' })
  .move(765, 175)
  .fill('none');

// text
draw
  .text('AC 입력')
  .move(13, 125)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 14 });
var cbtext1 = draw
  .text('CB1')
  .move(68, 65)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 14 });
var cbtext2 = draw
  .text('CB2')
  .move(68 + 160, 65)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 14 });
var cbtext4 = draw
  .text('CB4')
  .move(68 + 160 + 162, 203)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 14 });
var dracbtext5 = draw
  .text('CB5')
  .move(68 + 160 + 162 + 353, 65)
  .stroke({ width: 1, color: 'black' });
var dracbtext6 = draw
  .text('CB6')
  .move(68 + 160 + 162 + 353, 65 + 80)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 14 })
  .font({ size: 14 });
var dracbtext7 = draw
  .text('MC')
  .move(340, 20)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 14 });
draw
  .text('SID')
  .move(340, 125)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 14 });
draw
  .text('부하1')
  .move(780, 115)
  .stroke({ width: 0.7, color: 'black' })
  .font({ size: 12 });
draw
  .text('부하2')
  .move(780, 190)
  .stroke({ width: 0.7, color: 'black' })
  .font({ size: 12 });
draw
  .text('BAT')
  .move(490, 190)
  .stroke({ width: 1, color: 'black' })
  .font({ size: 16 });

//PL_1.fill('none');
function commandDraw() {
  //PL_1.fill('red');
  //alarm_ad14_set[11] 정전
  if (modbus_registor.value[14] & 0x0800 )
  {
    PL_1.fill('none');
    CB_1.fill('none');
    PL_2.fill('none');
  }
  else {
    PL_1.fill('red');
    CB_1.fill('red');
    PL_2.fill('red');
  }
  CB_1_old= CB_1.fill();
  PL2_old= PL_2.fill();
  if (modbus_registor.value[12] & 0x0100 ){
    CB_1.fill('none').move(50,-20);
    cbtext1.move(68, 65-20);
    PL_2.fill('none');
  }
  else {
    CB_1.fill(CB_1_old).move(50,0);
    cbtext1.move(68, 65);
    PL_2.fill(PL2_old);
  }

  if(modbus_registor.value[16] & 0x0030 ){
    batteryConverter.fill('none');
  }
  else
    batteryConverter.fill('red');

  if(modbus_registor.value[13] & 0x0001 ){
    PL_3.fill('red');
    CB_2.fill('red').move(215,0);;
    cbtext2.move(68 + 160, 65)
  }
  else{
    cbtext2.move(68 + 160, 65-20)
    CB_2.fill('none').move(215,-20);
    PL_3.fill('none');
  }

  PL_4.fill('red');
  PL_5.fill('red');
  PL_6.fill('red');
  PL_7.fill('red');
  PL_8.fill('red');
  CB_4.fill('red');
  CB_5.fill('red');
  CB_6.fill('red');

  SID_1.fill('red');
  SID_2.fill('red');
  SID_3.fill('red');
  SID_4.fill('red');
  SID_5.fill('red');

  SID_3.fill('red');
  MC_1.fill('red');
  MC_2.fill('red');
  MC_3.fill('red');
  MC_4.fill('red');

  PL_BAT.fill('red');
  //batteryBank.fill('red');
  PRL_1.fill('red');
  PRL_2.fill('red');
  PRL_3.fill('red');
  PRL_4.fill('red');

}

window.onload =  checkLoginStatus;
function checkLoginStatus() {
    // Get the cookie
  var loginCookie = getCookie("login");
  // If the cookie exists and is not expired, the user is logged in
  if (loginCookie && new Date() < new Date(loginCookie)) {
    console.log("login ok")
    return true;
  }
  // If the cookie does not exist or is expired, the user is not logged in
  window.location.href = "login.html";
}
      // Get a cookie by name
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
//const webSocket = new WebSocket('ws://'+ document.location.hostname+ ':81/');
webSocket = new WebSocket('ws://' + "192.168.0.57" + ':81/');
webSocket.onclose = (event) => {
    new Promise((resolve) => {
      setTimeout(resolve, 1000);
    }).then(() => {
      console.log('Socket Close');
      console.log('Socket retry connect....');
      webSocket = new WebSocket('ws://' + "192.168.0.57" + ':81/');
    }
  );

  // console.log("websocket connected...")
  // const data = JSON.stringify({ 'command_type': 'user', 'reg': 0, 'set': 0 }); /* BIT 12 =0 열림*/
  // console.log(data);
  // webSocket.send(data);
}

let el;
// document.getElementById('btnQuery1').addEventListener('click', (e) => {
//     const data = JSON.stringify({ 'command_type': 'modbus', 'startAddress:': 0,'number':60});
//     console.log("Query to server")
//     console.log(data);
//     webSocket.send(data);
// })
document.getElementById('HtmlMainView').style.display = 'block';
document.getElementById('HtmlLogView').style.display = 'none';
document.getElementById('viewBasic').style.display = 'none';
document.getElementById('viewSetNetwork').style.display = 'none';

//logout
document.getElementById('logout').addEventListener('click', (e) => {
 setCookie("login", "", new Date(0));
  window.location.href = "login.html";
});

document.getElementById('viewLog').addEventListener('click', (e) => {
  document.getElementById('HtmlMainView').style.display = 'none';
  document.getElementById('HtmlLogView').style.display = 'block';
  document.getElementById('viewBasic').style.display = 'none';
  document.getElementById('viewSetNetwork').style.display = 'none';
})
document.getElementById('htmlMain').addEventListener('click', (e) => {
  document.getElementById('HtmlMainView').style.display = 'block';
  document.getElementById('HtmlLogView').style.display = 'none';
  document.getElementById('viewBasic').style.display = 'none';
  document.getElementById('viewSetNetwork').style.display = 'none';
})
document.getElementById('viewB').addEventListener('click', (e) => {
  document.getElementById('HtmlMainView').style.display = 'none';
  document.getElementById('HtmlLogView').style.display = 'none';
  document.getElementById('viewBasic').style.display = 'block';
  document.getElementById('viewSetNetwork').style.display = 'none';
})
document.getElementById('viewN').addEventListener('click', (e) => {
  document.getElementById('HtmlMainView').style.display = 'none';
  document.getElementById('HtmlLogView').style.display = 'none';
  document.getElementById('viewBasic').style.display = 'none';
  document.getElementById('viewSetNetwork').style.display = 'block';
})
//btnSetTime
document.getElementById('btnSetTime').addEventListener('click', (e) => {
    nowTime = new Date();
    const data = JSON.stringify({ 'command_type': 'timeSet', 'reg': 0,'set':nowTime.getTime()/1000   }); /* BIT 12 =0 열림*/
    console.log(data);
    webSocket.send(data);
})



document.getElementById('btnQuery1').addEventListener('click', (e) => {
  if(modbus_registor.value[14] & 0x0800 ){
    document.getElementById('btnQuery1').innerHTML = '복전';
    modbus_registor.value[14] &= ~0x0800 ;
  }
  else 
  {
    document.getElementById('btnQuery1').innerHTML = '정전';
    modbus_registor.value[14] |= 0x0800 ;
  }
    const data = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 14,'set':modbus_registor.value[14] }); /* BIT 12 =0 열림*/
    console.log("Query to server")
    console.log(data);
    if(webSocket.readyState === webSocket.OPEN)
    webSocket.send(data);
    else console.log("socket was closed");
})
document.getElementById('btnQuery2').addEventListener('click', (e) => {
  if(modbus_registor.value[12] & 0x0100 ){
    document.getElementById('btnQuery2').innerHTML = 'CB1 OFF';
    modbus_registor.value[12] &= ~0x100 ;
  }
  else 
  {
    document.getElementById('btnQuery2').innerHTML = 'CB1 ON';
    modbus_registor.value[12] |= 0x0100 ;
  }
    const data = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 12,'set':modbus_registor.value[12] }); /* BIT 12 =0 열림*/
    console.log("Query to server")
    console.log(data);
    webSocket.send(data);
})
//충전기정지
document.getElementById('btnQuery3').addEventListener('click', (e) => {
  if(modbus_registor.value[16] & 0x0030 ){
    document.getElementById('btnQuery3').innerHTML = '충전기운전';
    modbus_registor.value[16] &= ~(0x0030) ;
  }
  else 
  {
    document.getElementById('btnQuery3').innerHTML = '충전기정지';
    modbus_registor.value[16] |= 0x0030 ;
  }
    const data = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 16,'set':modbus_registor.value[16] }); 
    console.log("Query to server")
    console.log(data);
    webSocket.send(data);
})
document.getElementById('btnQuery4').addEventListener('click', (e) => {
  if(modbus_registor.value[13] & 0x0001 ){
    document.getElementById('btnQuery4').innerHTML = 'CB2 OFF';
    modbus_registor.value[13] &= ~(0x0001) ;
  }
  else 
  {
    document.getElementById('btnQuery4').innerHTML = 'CB2 ON';
    modbus_registor.value[13] |= 0x0001 ;
  }
    const data = JSON.stringify({ 'command_type': 'ModBusSet', 'reg': 13,'set':modbus_registor.value[13] }); 
    console.log("Query to server")
    console.log(data);
    webSocket.send(data);
})
document.getElementById('btnTest').addEventListener('click', (e) => {
var sendString = document.getElementById('commandText').value;
console.log(sendString);
const data = JSON.stringify({'command_type': sendString, 'reg': 16,'set':modbus_registor.value[16]})
webSocket.send(data);
//document.getElementById('commandText').addEventListener('click', (e) => {
});
//
document.getElementById('btnLogRead').addEventListener('click', (e) => {
  webSocket.send("log_download");
});
//const data = new Uint8Array()

// function processReceivedData(data) {
//   // process the received data
//   var lines =[];
//   for(var i = 0;i< data.length; i+=110){
//     lines.push(data.slice(i,i+10));
//     console.log(i);
//   }
//   console.log(lines.length);
//   //var lines = data.split("\n");
//   var table = document.createElement("table");
//   for (var i = 0; i < lines.length; i++) {
//     var row = table.insertRow();
//     var cell = row.insertCell();
//     cell.innerHTML = lines[i];
//   }
//   document.body.appendChild(table);
//   // var dv = new DataView(lines);
//   // var logtime = dv.getFloat32(0, ture);
//   // logtime *= 1000;
//   // var data = [];
//   // for (var i = 0; i < 60; i++) {
//   //   data.push(dv.getInt16(4 + i * 2, tre));
//   // }
//   // var modBusData = {
//   //   logTime: logtime,
//   //   Data: data
//   // }
//   // console.log(modBusData.logTime);
// }
function printLogToHtml() {
  var table = document.createElement("table");
table.style.border = "1px solid black";
table.style.width = "100%";
table.style.borderCollapse = "collapse";

var thead = document.createElement("thead");
var tr = document.createElement("tr");
var th = document.createElement("th");
th.innerHTML = "Log Time";
th.style.border = "1px solid black";
tr.appendChild(th);

for (let i = 0; i < 60; i++) {
  var th = document.createElement("th");
  th.innerHTML = "ModBus" + i;
  th.style.border = "1px solid black";
  tr.appendChild(th);
}
thead.appendChild(tr);
table.appendChild(thead);

var tbody = document.createElement("tbody");
for (let i = 0; i < modBusDataArray.length; i++) {
  var tr = document.createElement("tr");
  var td = document.createElement("td");


    // nowTime = new Date(data.time*1000);
    // drawTime.text(nowTime.toLocaleString());

  td.innerHTML = (new Date(modBusDataArray[i].logTime*1000)).toLocaleString();
  td.style.border = "1px solid black";
  tr.appendChild(td);
  for (let j = 0; j < 60; j++) {
    var td = document.createElement("td");
    td.innerHTML = modBusDataArray[i].modBus60[j];
    td.style.border = "1px solid black";
    tr.appendChild(td);
  }
  tbody.appendChild(tr);
}
table.appendChild(tbody);
document.body.appendChild(table);

}
var isDownloadComplete = false;
var socketCommand="";
var modBusData_c;
async function isLogFunction(event) {
  if (event.data === "log_download") {
    //로그파일을 전송하라고 명령을 준다
    socketCommand = "log_download_start";
    receiveData="";
    //console.log("LogFile request start...");
    return;
  }
  if (event.data === "download_complete") {
    console.log("LogFile download complete..." + typeof(event.data) );

    isDownloadComplete = true;
    socketCommand = "";
    new Promise((resolve) => {
      setTimeout(resolve, 500);
    }).then(() => {

      printLogToHtml();
    for (let i = 0; i < modBusDataArray.length; i++) {
      console.log("modBusDataArray.length is "+modBusDataArray.length );
      console.log(modBusDataArray[i].logTime);
      console.log(modBusDataArray[i].modBus60);
    }

    });
    //processReceivedData(receiveData );
    return;
  }

  if (socketCommand === "log_download_start" || typeof(event.data) == Object) {
    //파일데이타가 도착했다.
    console.log("LogFile downloading..." + typeof(event.data) );
    reader = new FileReader();
    reader.readAsArrayBuffer(event.data);
    await new Promise(resolve => {
      reader.onload = (evt) => {
        modBusDataArray.push(new modBusDataClass(evt.target.result));
        //console.log("modBusDataArray.length is " + modBusDataArray.length);
        resolve();
      }
    });
    receiveData += event.data;
    return;
  }
}
webSocket.onmessage = async (event) => {
  await isLogFunction(event);
  let data;
  try {
    data = JSON.parse(event.data);
  } catch (error) {
    //console.error(error);
    return;
  }
  document.getElementById('statusBar').innerHTML = event.data;
  if (data.command_type == 'modbus') {
    nowTime = new Date(data.time*1000);
    drawTime.text(nowTime.toLocaleString());
    //console.log(nowTime.toLocaleString());
    modbus_registor.value = data.value;
    el = document.getElementById('modbusData');
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
    loadRate.innerHTML = data.value[32]+"(%)";

    //inputAmp_R
    // for (var key in data) {
    //   el.innerHTML += '[' + key + ']=' + data[key] + '<br>';
    // }
  }
  commandDraw();
};
  // else if (data.command_type == 'time') {
  //   console.log(data.time);
  //   nowTime = new Date(1000 * data.time);
  //   console.log(nowTime.toLocaleString());
  //   drawTime.text(nowTime.toLocaleString());
  // }



















 
    //    function fade(element) {
    //        var op = 1;  // initial opacity
    //        var timer = setInterval(function () {
    //            if (op <= 0.1){
    //                clearInterval(timer);
    //                element.style.display = 'none';
    //            }
    //            element.style.opacity = op;
    //            element.style.filter = 'alpha(opacity=' + op * 100 + ")";
    //            op -= op * 0.1;
    //        }, 100);
    //    }
 
    //    function unfade(element) {
    //        var op = 0.1;  // initial opacity
    //        element.style.display = 'block';
    //        var timer = setInterval(function () {
    //            if (op >= 1){
    //                clearInterval(timer);
    //            }
    //            element.style.opacity = op;
    //            element.style.filter = 'alpha(opacity=' + op * 100 + ")";
    //            op += op * 0.1;
    //        }, 100);
    //    }
 
    //    webSocket.onmessage = (event) => {
    //        const data = JSON.parse(event.data);
    //        //console.log(data);
    //        if (data.command_type== 'time') {
    //            el = document.getElementById('time');
    //            el.innerHTML = 'Time on the Server: ' + data.time;
    //        }
    //        if (data.command_type== 'message') {
    //            // Grab the wishing-well
    //            console.log(data)
    //            const palette = document.getElementById('wishing-well');
 
    //            // create new div with position and innerHtml
    //            let msgContainer = document.createElement("div");
    //            msgContainer.id = "message"
    //            msgContainer.style.position = "absolute";
    //            //console.log("position: ", data.position)
    //            msgContainer.style.top = data.position.y + "px";
    //            msgContainer.style.left = data.position.x + "px";
    //            msgContainer.style.opacity = 0;
    //            msgContainer.innerHTML = data.message;
 
    //            // add the new div to wishing-well
    //            palette.appendChild(msgContainer);
    //            unfade(msgContainer)
 
    //            // fire off fade/delete new div with a delay in the beginning
    //            async function fadeAndDelete() {
    //                setTimeout(() => {
    //                    fade(msgContainer);
    //                    setTimeout(() => {
    //                        let elem = document.getElementById("message");
    //                        elem.remove();
    //                    }, 1100)
    //                }, 4000);
    //            }
              
    //            fadeAndDelete();
    //        }
    //    };
  // const data = JSON.stringify(
  //   {
  //     'command_type': 'logRead',
  //     'reg': 0,
  //     'set': modbus_registor.value[1]
  //   });
  // console.log("Query to server")
  // console.log(data);
    // const int16Array = new Int16Array(dataArrayBuffer);
    // const arrayBuffer = int16Array.buffer;
// const ConStBitInfo = [
//   { bit: 0, message: "Converter Overheat" },
//   { bit: 2, message: "Input MC Status (Closed)" }, 
//   { bit: 3, message: "Main FUSE Fail" }, 
//   { bit: 5, message: "Pluse Ground Fault" }, 
//   { bit: 6, message: "Minus Ground Fault" }, 
//   { bit: 8, message: "Low Battery Voltage" }, 
//   { bit: 9, message: "CB1 Status (Closed)" }, 
//   { bit: 10, message: "CB2 Status (Closed)" }, 
//   { bit: 11, message: "CB3 Status (Closed)" }, 
//   { bit: 12, message: "Converter_R_GDU Fault" }, 
//   { bit: 13, message: "Converter_S_GDU Fault" }, 
//   { bit: 14, message: "Converter_T_GDU Fault" }
// ];
