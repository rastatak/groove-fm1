let dF,dLfo,dLg,ctx,master,mFilt,hFilt,dly,rev,noise,drumBus,mSh,mSG,xD,xDl,xR,mShV=-1;
const bufs={},V=[],rowG=[],HIT={}; /* HIT : heure du dernier coup de chaque piste (voyants de la vue FM-1) */
const hz=m=>440*Math.pow(2,(m-69)/12);
// Kit de secours synthétisé (utilisé tant qu'aucun WAV n'est choisi pour la ligne)
const DR=[{f:[150,45],d:.35},{f:[200,150],d:.12,n:.7,hp:1800},{n:1,d:.17,bp:1300,clap:1},{n:.6,d:.045,hp:7500},{n:.6,d:.3,hp:7500},{f:[850,420],d:.08},{f:[120,65],d:.3},{f:[1700,1650],d:.03,sq:1}];

const CRV={};
/* disto (tanh) + bitcrush (paliers : 1 = 3 niveaux) */
function curve(k,cr){cr=cr||0;const key=k.toFixed(3)+'|'+cr.toFixed(2);if(CRV[key])return CRV[key];const n=1024,c=new Float32Array(n),a=1+k*40,q=cr>0?Math.round(2+Math.pow(1-cr,2)*60):0;
  for(let i=0;i<n;i++){const x=i/(n-1)*2-1;let y=Math.tanh(x*a)/Math.tanh(a);if(q)y=Math.round(y*q)/q;c[i]=y;}return CRV[key]=c;}
/* un bus : disto/crush → gate (hachage) → volume → sorties, écho, reverb et chorus (délai court modulé) */
function bus(){const i=ctx.createGain(),sh=ctx.createWaveShaper(),gt=ctx.createGain(),o=ctx.createGain(),d=ctx.createGain(),r=ctx.createGain(),cg=ctx.createGain(),cd=ctx.createDelay(.05),cl=ctx.createOscillator(),cm=ctx.createGain();
  i.connect(sh);sh.connect(gt);gt.connect(o);o.connect(master);o.connect(d);d.connect(dly);o.connect(r);r.connect(rev);
  cg.gain.value=0;cd.delayTime.value=.014;cl.frequency.value=.7;cm.gain.value=.006;cl.connect(cm);cm.connect(cd.delayTime);cl.start();o.connect(cg);cg.connect(cd);cd.connect(master);return{i,sh,gt,o,d,r,cg,cd};}

function initAudio(){
  if(ctx)return;
  ctx=new (window.AudioContext||window.webkitAudioContext)();
  master=ctx.createGain();master.gain.value=.8;
  mFilt=ctx.createBiquadFilter();mFilt.type='lowpass';
  hFilt=ctx.createBiquadFilter();hFilt.type='highpass';
  const comp=ctx.createDynamicsCompressor();master.connect(mFilt);mFilt.connect(hFilt);comp.connect(ctx.destination);
  /* Effets généraux de la vue LIVE, pris après les filtres : disto, puis écho et reverb */
  mSh=ctx.createWaveShaper();mSG=ctx.createGain();hFilt.connect(mSh);mSh.connect(mSG);mSG.connect(comp);
  xD=ctx.createGain();xD.gain.value=0;xDl=ctx.createDelay(2);const xf=ctx.createGain();xf.gain.value=.5;mSG.connect(xD);xD.connect(xDl);xDl.connect(xf);xf.connect(xDl);xDl.connect(comp);
  dly=ctx.createDelay(2);const fb=ctx.createGain();fb.gain.value=.45;dly.connect(fb);fb.connect(dly);dly.connect(master);
  rev=ctx.createConvolver();const L=Math.floor(ctx.sampleRate*2.5),ir=ctx.createBuffer(2,L,ctx.sampleRate);
  for(let c=0;c<2;c++){const d=ir.getChannelData(c);for(let i=0;i<L;i++)d[i]=(Math.random()*2-1)*Math.pow(1-i/L,3);}
  rev.buffer=ir;const rg=ctx.createGain();rg.gain.value=.6;rev.connect(rg);rg.connect(master);
  xR=ctx.createGain();xR.gain.value=0;const rv2=ctx.createConvolver();rv2.buffer=ir;mSG.connect(xR);xR.connect(rv2);rv2.connect(comp);
  noise=ctx.createBuffer(1,ctx.sampleRate,ctx.sampleRate);const nd=noise.getChannelData(0);for(let i=0;i<nd.length;i++)nd[i]=Math.random()*2-1;
  drumBus=bus();
  /* un gain par ligne de batterie : le volume se règle en direct, même sur un son déjà lancé */
  /* filtre de toute la batterie (page LIVE · FX LIVE) : passe-bas + résonance, avec son LFO (wobble) */
  dF=ctx.createBiquadFilter();dF.type='lowpass';dF.connect(drumBus.i);dLfo=ctx.createOscillator();dLg=ctx.createGain();dLg.gain.value=0;dLfo.connect(dLg);dLg.connect(dF.detune);dLfo.start();
  ROWS.forEach((_,r)=>{rowG[r]=ctx.createGain();rowG[r].connect(dF);});
  S.tracks.forEach((tr,i)=>{const b=bus(),v={b};
    if(tr.type!=='pad'){v.osc=ctx.createOscillator();v.f=ctx.createBiquadFilter();v.f.type='lowpass';v.g=ctx.createGain();v.g.gain.value=0;
      v.osc.connect(v.f);v.f.connect(v.g);v.g.connect(b.i);v.osc.start();}
    V[i]=v;});
  applyParams();
}

function applyParams(){
  if(!ctx)return;const now=ctx.currentTime;
  master.gain.setTargetAtTime(S.mv==null?.8:S.mv,now,.02);
  mFilt.frequency.setTargetAtTime(80*Math.pow(2,S.mf*7.9),now,.03);
  hFilt.frequency.setTargetAtTime(20*Math.pow(2,(S.hp||0)*8),now,.03);mFilt.Q.value=hFilt.Q.value=1+(S.mq||0)*14;
  dly.delayTime.setTargetAtTime(60/S.bpm*.75,now,.05);
  xDl.delayTime.setTargetAtTime(60/S.bpm*.75,now,.05);xD.gain.setTargetAtTime((S.xd||0)*.8,now,.03);xR.gain.setTargetAtTime((S.xr||0)*.8,now,.03);
  if(mShV!==(S.xs||0)){mShV=S.xs||0;mSh.curve=mShV?curve((mShV*12-1)/40):null;mSG.gain.setTargetAtTime(1-.55*mShV,now,.03);}
  const set=(b,p,mute)=>{b.sh.curve=curve(p.dist||0,p.cr);b.o.gain.setTargetAtTime(mute?0:p.vol,now,.01);b.d.gain.value=p.dly;b.r.gain.value=p.rev;b.cg.gain.setTargetAtTime((p.cho||0)*.9,now,.03);};
  set(drumBus,S.dfx,false);
  {const d=S.dfx,c=d.cut==null?1:d.cut;dF.frequency.setTargetAtTime(c>=.995?22000:60*Math.pow(2,c*8.4),now,.03);dF.Q.value=.7+(d.res||0)*16;
   dLfo.frequency.setTargetAtTime(.1*Math.pow(200,d.lfr==null?.3:d.lfr),now,.05);dLg.gain.setTargetAtTime((d.lfd||0)*2400,now,.05);}
  S.rows.forEach((r,k)=>{if(rowG[k])rowG[k].gain.setTargetAtTime(r.vol,now,.015);});
  S.tracks.forEach((t,i)=>{const v=V[i];if(!v)return;set(v.b,t,!aud('t',i));lfoSet(v,t,now);if(v.osc){v.osc.type=t.wave;v.f.type=t.ftype||'lowpass';v.f.Q.value=t.res*22;v.osc.detune.setTargetAtTime((t.po||0)*1200,now,.02);}});
}

/* LFO de la piste : module la coupure du filtre (en cents) · forme, vitesse libre (0,1-20 Hz) ou calée sur le tempo, profondeur */
const LFW=['sine','triangle','square','sawtooth'],LFD=[4,2,1,.5,.25];
const lfoHz=t=>t.lfs?(S.bpm/60)/LFD[Math.min(4,Math.round((t.lfr||0)*4))]:.1*Math.pow(200,t.lfr==null?.3:t.lfr);
function lfoSet(v,t,now){if(!v.lfo){v.lfo=ctx.createOscillator();v.lg=ctx.createGain();v.lg.gain.value=0;v.lfo.connect(v.lg);v.lfo.start();if(v.f)v.lg.connect(v.f.detune);
    v.an=ctx.createAnalyser();v.an.fftSize=1024;v.b.o.connect(v.an);}
  v.lfo.type=LFW[t.lfw||0];v.lfo.frequency.setTargetAtTime(lfoHz(t),now,.05);v.lg.gain.setTargetAtTime((t.lfd||0)*2400,now,.05);}
function d2m(tr,d){const sc=SCALES[S.scale],L=sc.length;return S.root+12*(tr.oct+1)+sc[((d%L)+L)%L]+12*Math.floor(d/L);}

// Joue le degré d de la gamme sur la piste i (dur=0 : note tenue jusqu'à off)
function note(i,d,t,dur,acc,slide,gm=1){
  const tr=S.tracks[i],v=V[i];if(!v||!ctx)return;HIT['t'+i]=t;
  /* Sortie USB MIDI : la piste joue sur le FM-1 (seulement, ou en plus du son interne) */
  if(tr.mo&&midiNoteOut(i,tr,d,t,dur,acc,gm)&&tr.mo===1)return;
  const cut=60*Math.pow(2,tr.cut*7.5),pk=Math.min(18000,cut*Math.pow(2,tr.env*(acc?5:3.5))),lvl=(acc?.5:.33)*gm;
  const rv=rvI===i||!!lv('t').rv,rw=rv&&dur>0,sid=tr.smp==='piano'?pno(d2m(tr,d)+(tr.po||0)*12):tr.smp,sb=sid&&bufs[sid];if(sb)return smpNote(tr,v,rv?rbuf(sid):sb,d,t,dur,acc,slide,gm,cut,pk,rv,sid);
  if(tr.type==='pad'){const du=dur||.8;[0,...(SCP[S.scale]||SCP.Mineur).ch].forEach((k,j)=>{const o=ctx.createOscillator(),g=ctx.createGain(),fl=ctx.createBiquadFilter();
      o.type=tr.wave;o.frequency.value=hz(d2m(tr,d+k));o.detune.value=(j-1)*9+(tr.po||0)*1200;fl.type=tr.ftype||'lowpass';fl.frequency.value=cut;fl.Q.value=tr.res*10;if(v.lg)v.lg.connect(fl.detune);
      g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(.14,t+.02+(tr.att==null?.2:tr.att*1.5));g.gain.setTargetAtTime(0,t+du,.4);
      o.connect(fl);fl.connect(g);g.connect(v.b.i);o.start(t);o.stop(t+du+2.5);});return;}
  const f=hz(d2m(tr,d)),fr=v.osc.frequency;
  if(v.sl){fr.setTargetAtTime(f,t,.025);}
  else{
    fr.cancelScheduledValues(t);
    if(tr.type==='fx'){fr.setValueAtTime(rw?f:f*4,t);fr.exponentialRampToValueAtTime(rw?f*4:f,t+(rw?dur:.12+tr.dec*.4));}else if(tr.gl>0)fr.setTargetAtTime(f,t,.004+tr.gl*.18);else fr.setValueAtTime(f,t);
    v.g.gain.cancelScheduledValues(t);
    /* Reverse tenu : enveloppe à l'envers, le son monte puis se coupe net */
    if(rw){v.g.gain.setValueAtTime(lvl*.08,t);v.g.gain.linearRampToValueAtTime(lvl,t+dur);}else v.g.gain.setTargetAtTime(lvl,t,.003+(tr.att||0)*.15);
    v.f.frequency.cancelScheduledValues(t);
    if(rw){v.f.frequency.setValueAtTime(cut,t);v.f.frequency.exponentialRampToValueAtTime(pk,t+dur);}
    else{v.f.frequency.setValueAtTime(pk,t);v.f.frequency.setTargetAtTime(cut,t,.03+tr.dec*.5);}
  }
  v.sl=slide?1:0;
  if(!slide&&dur)v.g.gain.setTargetAtTime(0,t+dur,.012);
}
function off(i,t){const v=V[i];if(!v)return;if(S.tracks[i].mo)midiOff(i,t);cutV(v,t);v.sl=0;if(v.g){v.g.gain.cancelScheduledValues(t);v.g.gain.setTargetAtTime(0,t,.02);}}

// Son importé (WAV) joué à la hauteur de la note : la « note du son » la plus proche de la tonique = vitesse d'origine
// Piano interne : un son par octave (Do1 à Do7), calculé à la première note puis rejoué à la hauteur voulue
function pno(m){const k=Math.max(0,Math.min(6,Math.round((m-24)/12))),id='pn'+k;
  if(!bufs[id]){const sr=ctx.sampleRate,f=hz(24+12*k),N=Math.floor(sr*(3.5-k*.4)),b=ctx.createBuffer(1,N,sr),d=b.getChannelData(0);
    /* cordes : harmoniques légèrement décalées, les aiguës s'éteignent plus vite, deux cordes un peu désaccordées */
    for(let n=1;n<=24;n++){const fn=n*f*Math.sqrt(1+.0004*n*n);if(fn>sr*.3)break;
      const a=Math.pow(n,-1.15)*(.15+Math.abs(Math.sin(n*Math.PI/8))),q=Math.exp(-(1+.35*(n-1))*3/N);
      [1,1.0011].forEach(dt=>{const w=2*Math.PI*fn*dt/sr,c=2*Math.cos(w);let y1=-Math.sin(w),y2=-Math.sin(2*w),e=a;
        for(let i=0;i<N;i++){const y=c*y1-y2;y2=y1;y1=y;d[i]+=y*e;e*=q;}});}
    /* coup de marteau, puis mise à niveau et fin en fondu */
    for(let i=0;i<sr*.02;i++)d[i]+=(Math.random()*2-1)*.3*Math.exp(-i/(sr*.003));
    let p=0;for(let i=0;i<N;i++)p=Math.max(p,Math.abs(d[i]));
    for(let i=0;i<N;i++)d[i]*=.65/(p||1)*Math.min(1,(N-i)/(sr*.05));
    bufs[id]=b;}
  return id;}
function smpNote(tr,v,b,d,t,dur,acc,slide,gm,cut,pk,rv,sid){
  const pn=tr.smp==='piano',pad=tr.type==='pad'&&!pn,dl=((S.root-(tr.sn||0)+18)%12)-6,so=tr.so==null?tr.oct:tr.so;
  /* piano : vitesse calculée depuis l'octave du son utilisé (k) */
  const pr=k=>Math.min(8,Math.max(.06,Math.pow(2,(d2m(tr,d)+(tr.po||0)*12-24-12*k)/12)));
  const rate=pn?pr(+sid[2]):Math.min(8,Math.max(.06,Math.pow(2,(d2m(tr,d)-(S.root+12*(so+1)-dl))/12+(tr.po||0))));
  const hold=dur?dur+(pad?0:tr.dec*1.2):0,open=tr.cut>=.99;
  if(v.sl&&v.sv&&!pad){v.sv.s.playbackRate.setTargetAtTime(pn&&v.sv.k!=null?pr(v.sv.k):rate,t,.025);}
  else{
    cutV(v,t);
    const s=ctx.createBufferSource(),fl=ctx.createBiquadFilter(),g=ctx.createGain();
    s.buffer=b;s.loop=pad;s.playbackRate.value=rate;fl.type=open?'lowpass':(tr.ftype||'lowpass');fl.Q.value=tr.res*(pad?10:22);if(v.lg&&!open)v.lg.connect(fl.detune);
    fl.frequency.setValueAtTime(open?20000:(pad?cut:pk),t);if(!open&&!pad)fl.frequency.setTargetAtTime(cut,t,.03+tr.dec*.5);
    g.gain.setValueAtTime(0,t);g.gain.setTargetAtTime((acc?1:.75)*gm,t,(pad?.08:.003)+(tr.att||0)*.15);
    const D=b.duration,st0=Math.min(D*.95,(tr.ss||0)*D),o0=rv?(pad?0:Math.max(0,D-st0-(hold||1)*rate)):st0; /* reverse : on joue à l'envers le même morceau du fichier */
    if(pad){s.loopStart=rv?0:st0;s.loopEnd=rv?D-st0:D;}
    s.connect(fl);fl.connect(g);g.connect(v.b.i);s.start(t,o0);v.sv={s,g,k:pn?+sid[2]:null};
  }
  v.sl=slide?1:0;
  if(!slide&&hold){const x=v.sv;x.g.gain.setTargetAtTime(0,t+hold,pad?.3:pn?.08:.03);try{x.s.stop(t+hold+(pad?2:.4));}catch(e){}}
}
// Copie à l'envers d'un son importé (calculée une fois, pour le Reverse)
const rbufs={};
function rbuf(id){const b=bufs[id];let r=rbufs[id];
  if(!r||r.src!==b){const n=ctx.createBuffer(b.numberOfChannels,b.length,b.sampleRate);
    for(let c=0;c<b.numberOfChannels;c++){const d=n.getChannelData(c);d.set(b.getChannelData(c));d.reverse();}
    r=rbufs[id]={src:b,b:n};}
  return r.b;}
function cutV(v,t){const x=v.sv;if(!x)return;v.sv=null;try{x.g.gain.cancelScheduledValues(t);x.g.gain.setTargetAtTime(0,t,.015);x.s.stop(t+.12);}catch(e){}}

// rv > 0 : coup joué à l'envers dans une fenêtre de rv secondes (l'attaque tombe à la fin)
function drum(r,t,vel,gm=1,rv=0){
  const row=S.rows[r];if(!aud('r',r)||!ctx)return;HIT['r'+r]=t+rv;
  const mo=rOut(r);if(mo&&midiDrumOut(r,t+rv,vel,gm)&&mo===1)return;
  const g=ctx.createGain();g.gain.value=(vel>1?1.25:.85)*gm;g.connect(rowG[r]||drumBus.i);
  const b=bufs[row.smp];
  if(b){const s=ctx.createBufferSource();
    if(rv){const x=Math.min(b.duration,rv);s.buffer=rbuf(row.smp);s.connect(g);s.start(t+rv-x,b.duration-x);return;}
    s.buffer=b;s.connect(g);s.start(t);return;}
  const p=DR[r],d=rv?Math.min(p.d,rv):p.d;if(rv)t+=rv-d;
  if(p.f){const o=ctx.createOscillator(),og=ctx.createGain();o.type=p.sq?'square':'sine';
    if(rv){o.frequency.setValueAtTime(p.f[1],t);o.frequency.setValueAtTime(p.f[1],t+d*.5);o.frequency.exponentialRampToValueAtTime(p.f[0],t+d);}else{o.frequency.setValueAtTime(p.f[0],t);o.frequency.exponentialRampToValueAtTime(p.f[1],t+p.d*.5);}
    if(rv){og.gain.setValueAtTime(.001,t);og.gain.exponentialRampToValueAtTime(p.sq?.4:1,t+d);og.gain.setTargetAtTime(0,t+d,.004);}else{og.gain.setValueAtTime(p.sq?.4:1,t);og.gain.exponentialRampToValueAtTime(.001,t+p.d);}
    o.connect(og);og.connect(g);o.start(t);o.stop(t+d+.05);}
  if(p.n){const s=ctx.createBufferSource(),fl=ctx.createBiquadFilter(),ng=ctx.createGain();s.buffer=noise;
    fl.type=p.bp?'bandpass':'highpass';fl.frequency.value=p.bp||p.hp;
    if(p.clap&&!rv)for(let k=0;k<3;k++){ng.gain.setValueAtTime(p.n,t+k*.012);ng.gain.exponentialRampToValueAtTime(.05,t+k*.012+.01);}
    const t0=p.clap&&!rv?t+.036:t;
    if(rv){ng.gain.setValueAtTime(.001,t);ng.gain.exponentialRampToValueAtTime(p.n,t+d);ng.gain.setTargetAtTime(0,t+d,.004);}else{ng.gain.setValueAtTime(p.n,t0);ng.gain.exponentialRampToValueAtTime(.001,t0+p.d);}
    s.connect(fl);fl.connect(ng);ng.connect(g);s.start(t,Math.random()*.5);s.stop(t0+d+.05);}
}

// Stockage local des WAV (IndexedDB du navigateur)
function idb(mode,fn){return new Promise((res,rej)=>{try{const rq=indexedDB.open('fm1groove',1);
  rq.onupgradeneeded=()=>rq.result.createObjectStore('smp');
  rq.onsuccess=()=>{const tx=rq.result.transaction('smp',mode),r=fn(tx.objectStore('smp'));tx.oncomplete=()=>res(r&&r.result);tx.onerror=()=>rej(tx.error);};
  rq.onerror=()=>rej(rq.error);}catch(e){rej(e);}});}
