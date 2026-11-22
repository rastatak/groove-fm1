const ROWS=['KICK','SNARE','CLAP','HAT F.','HAT O.','PERC','LOW','RIM'];
const NOTES=['Do','Do#','Ré','Ré#','Mi','Fa','Fa#','Sol','Sol#','La','La#','Si'];
const SCALES={
  'Mineur':[0,2,3,5,7,8,10],
  'Phrygien':[0,1,3,5,7,8,10],
  'Dorien':[0,2,3,5,7,9,10],
  'Mineur harmonique':[0,2,3,5,7,8,11],
  'Penta mineure':[0,3,5,7,10],
  'Majeur':[0,2,4,5,7,9,11]
};
// Profil de chaque gamme pour le générateur : sy = chance de contre-temps, ch = degrés de la tierce et de la quinte, co = notes de couleur
const SCP={
  'Mineur':{sy:.18,ch:[2,4],co:[3,5]},
  'Phrygien':{sy:.22,ch:[2,4],co:[1,3]},
  'Dorien':{sy:.32,ch:[2,4],co:[5,3]},
  'Mineur harmonique':{sy:.1,ch:[2,4],co:[6,5]},
  'Penta mineure':{sy:.4,ch:[1,3],co:[2,4]},
  'Majeur':{sy:.25,ch:[2,4],co:[5,1]}
};
// Modèles : "Nom|8 lignes en hexadécimal" (KICK SNARE CLAP HAT-F HAT-O PERC LOW RIM), 16 pas par ligne
const STYLES={
  techno:{bpm:136,t:['Pulse|8888 0000 0808 BBBB 2222 0124 0000 0000','Warehouse|8888 0000 0808 2222 0000 4212 1111 0240','Drive|888A 0000 0808 AAAA 2222 0090 0000 1248','Rumble|8888 0000 0800 2222 0002 0000 6666 0000']},
  tekno:{bpm:175,t:['Teuf|8888 0808 0000 FFFF 2222 0000 0000 0000','Sound system|8889 0808 0000 AAAA 2222 1010 0003 0000','Hardfloor|8A88 0808 0001 7777 2222 0000 0000 4040','Roulement|8888 080B 0000 FFFF 2222 0000 0020 0000']},
  trance:{bpm:138,t:['Classic|8888 0000 0808 DDDD 2222 0000 0000 0000','Uplift|8888 0808 0808 5555 2222 0102 0000 1010','Roll|8888 000F 0808 FFFF 2222 0000 0000 0000']},
  psytrance:{bpm:145,t:['Full on|8888 0808 0000 7777 2222 0410 0000 0000','Forest|8888 0808 0000 7777 2222 0000 0101 1212','Goa|8888 0800 0008 BBBB 2222 4040 0000 0005']},
  acidcore:{bpm:170,t:['Acid raid|8888 0808 0808 AAAA 2222 0000 0000 0202','Broken|8A8A 0808 0000 FFFF 2222 0101 0000 0000','Mental|888B 0808 0000 6666 2222 1000 0000 0040']},
  minimal:{bpm:126,t:['Click|8888 0000 0808 2222 0000 0420 0000 1204','Micro|8888 0000 0800 2A2A 0002 0041 0100 4010','Groove|8888 0000 0008 6666 0202 1040 0000 0490']},
  dub:{bpm:122,t:['One drop|0080 0000 0000 AAAA 0002 0000 0000 0080','Steppers|8888 0080 0000 2222 0000 0000 0000 0808','Dub techno|8888 0000 0808 2222 0202 0004 0000 1040']},
  /* Styles ajoutés — sc = gamme associée, appliquée quand on choisit le style dans un menu « Style » */
  'hip-hop':{bpm:90,sc:'Penta mineure',t:['Boom bap|90A0 0808 0000 AAAA 0000 0000 0000 0000','Backbeat|8281 0808 0000 AAAA 0008 0000 0000 0000','Groove|8210 0080 0000 AAAA 0002 0000 0000 0000','Ghost|A020 0808 0000 AAAA 0000 0000 0000 1041']},
  trap:{bpm:140,sc:'Mineur',t:['Bounce|8210 0080 0080 AAAA 0000 0000 8210 0000','Club|8250 0080 0080 ABAB 0000 0000 8010 0000','Atlanta|8120 0000 0808 77D7 0020 0000 8020 0000','Lent|8004 0080 0000 8888 0000 0000 8000 0000','Doubles|8020 0080 0080 FFFF 0000 0000 8000 0000']},
  drill:{bpm:142,sc:'Mineur harmonique',t:['UK|8448 0080 0000 9292 0000 0000 8408 0000','Temps 4|8440 0008 0000 9293 0200 0000 8040 0000','Fantôme|8220 0080 0000 B296 0000 0000 8020 0100']},
  schranz:{bpm:155,sc:'Phrygien',t:['Loop|8888 0000 0808 FFFF 2222 4A4A 0000 0000','Marteau|8888 0808 0808 FFFF 2222 0000 6666 0000','Ride|8888 0000 0808 BBBB AAAA 0000 0000 4124','Compress|888A 0808 0000 FFFF 2222 2424 0101 0000']},
  acid:{bpm:140,sc:'Phrygien',t:['Squat|8888 0000 0808 AAAA 2222 0000 0000 0000','Spirale|8888 0000 0808 FFFF 2222 0000 0002 4040','Trip|888A 0000 0808 BBBB 2222 0020 0000 0000','Minimal 303|8888 0000 0008 2222 0000 0000 0000 0490']},
  'acid house':{bpm:122,sc:'Penta mineure',t:['Jack trax|8888 0000 0808 AAAA 2222 0000 0000 1212','Rave 88|8888 0808 0000 FFFF 2222 0000 0041 0000','Chicago|8888 0000 0808 BBBB 2222 0000 0000 0000','Cowbell|8888 0000 0808 2222 0000 9224 0000 0000']},
  house:{bpm:124,sc:'Dorien',t:['Classic|8888 0000 0808 2222 0000 0000 0000 0000','Open hat|8888 0000 0808 8888 2222 0000 0000 0000','Jack|8888 0000 0808 AAAA 2222 0000 0000 0124','Deep|8888 0000 0808 2222 0002 0410 0000 0000']},
  hardcore:{bpm:180,sc:'Mineur',t:['Gabber|8888 0000 0808 0000 2222 0000 0000 0000','Rotterdam|8888 0000 0808 FFFF 2222 0000 0000 0000','Frenchcore|8888 0000 0808 0000 2222 0000 2222 0000','Terror|AAAA 0808 0000 FFFF 2222 0000 0000 0000','Kick roll|888F 0000 0808 AAAA 2222 0000 0000 0000','Industriel|8A82 0808 0000 AAAA 0000 0000 0000 4040']},
  hardtek:{bpm:190,sc:'Mineur',t:['Kick bass|8888 0808 0000 2222 0000 0000 2222 0000','Rebond|8888 0808 0000 AAAA 2222 0000 3333 0000','Roulé|888B 0808 0000 FFFF 2222 0000 2220 0000','Raggatek|8888 0808 0000 2222 0000 0000 2222 1212']},
  tribe:{bpm:170,sc:'Penta mineure',t:['Free party|8888 0808 0000 2222 0000 0000 0000 0000','Tribal|8888 0808 0000 2222 0000 4242 0101 0000','Sautillant|8A88 0808 0000 AAAA 2222 0000 0202 0000','Congas|8888 0008 0800 6666 2222 9494 0000 0000']}
};
// Progressions d'accords par gamme : degré de la fondamentale, un accord par mesure (ex. Mineur : i – VI – III – VII)
const PROGS={
  'Mineur':[[0,5,2,6],[0,6,5,6],[0,3,5,6],[0,5,6,0],[0,2,6,5]],
  'Phrygien':[[0,1,0,6],[0,5,1,0],[0,0,1,6]],
  'Dorien':[[0,3,0,6],[0,2,6,3],[0,6,3,0]],
  'Mineur harmonique':[[0,3,4,0],[0,5,3,4],[0,0,4,4]],
  'Penta mineure':[[0,4,1,4],[0,1,2,4],[0,0,2,3]],
  'Majeur':[[0,4,5,3],[5,3,0,4],[0,5,3,4],[0,3,4,3]]
};
// Rythmes du générateur, toujours sur la grille des 16 pas : a = droits, b = syncopés (contre-temps)
const RH={
  bass:{a:['x.x.x.x.x.x.x.x.','..x...x...x...x.','..xx..xx..xx..xx','x.xxx.xxx.xxx.xx','x.x.x.xxx.x.x.xx'],
    b:['x..x..x.x..x..x.','x..x..x...x.x...','..x..x.x..x..x.x','x..x.x..x..x.x..','x.x..x.x..x.x.x.']},
  synth:{a:['x.x.x.x.x.x.x.x.','xxxxxxxxxxxxxxxx','x.xxx.xxx.xxx.xx','x...x.x.x...x.x.','x.x.x...x.x.x...','x.xxx...x.xxx...'],
    b:['x..x..x.x..x..x.','x..x..x..x..x.x.','..x.x..x..x.x..x','x.x..x.x..x..x.x','x..x.x..x.x..x..','x..x..x...x..x..']}
};
const grid=s=>s.trim().split(/\s+/).map(h=>{const n=parseInt(h,16);return Array.from({length:16},(_,i)=>(n>>(15-i))&1);});
const stepsNew=()=>Array.from({length:16},()=>({n:-1,a:0,s:0}));
const TRK=(name,type,o)=>Object.assign({name,type,mute:false,vol:.7,oct:2,wave:'sawtooth',cut:.4,res:.65,env:.6,dec:.3,dist:.25,dly:.1,rev:.05,po:0,lp:0,rl:1,ar:0,bw:0,smp:'',sn:0,ss:0,steps:stepsNew()},o);
// Piste SYNTHÉ 2 (aussi ajoutée aux anciens projets au chargement)
const SYN2=()=>TRK('SYNTHÉ 2','synth',{oct:5,cut:.5,res:.35,dist:.05,dly:.3,rev:.25,vol:.4});
const defState=()=>({bpm:136,swing:0,root:9,scale:'Mineur',style:'techno',auto:0,cur:0,sel:0,mf:1,prog:1,lib:[],mels:[],hp:0,mq:0,solo:[],gm:{r:0,t:0},lt:'',xd:0,xr:0,xs:0,am:{n:4,on:1,d:{}},
  dfx:{vol:.9,dist:0,dly:0,rev:.06},
  rows:ROWS.map(()=>({mute:false,vol:.8,smp:'',lp:0,rl:1,ar:0})),
  rhythms:[{name:'Techno · Pulse',tmp:false,g:grid(STYLES.techno.t[0].split('|')[1])}],
  tracks:[
    TRK('BASSE','bass',{}),
    TRK('SYNTHÉ','synth',{oct:4,wave:'square',cut:.55,res:.3,dist:.05,dly:.4,rev:.2,vol:.45}),
    TRK('BASSE 2','bass',{oct:3,wave:'square',vol:.5}),
    SYN2(),
    TRK('NAPPE','pad',{oct:4,cut:.5,res:.1,dist:0,rev:.7,vol:.6}),
    TRK('FX','fx',{oct:5,wave:'triangle',dist:0,dly:.6,rev:.5,vol:.4})
  ]});
