/* =====================================================================
   VUE FM-1 : l'app sur un FM-1 virtuel (écran 240×240, 4 knobs, SELECT / PRESETS / ALGORITHM / MASTER,
   12 boutons, OCT-/OCT+, 27 touches). Mêmes données et même moteur que l'éditeur.
   Règles d'UX, partout pareilles :
     · PRESETS choisit la piste (8 lignes de batterie + 6 pistes mélodiques)
     · les 11 touches du HAUT jouent la piste choisie (batterie : les 8 sons ; mélodie : la gamme)
     · les 16 touches du BAS changent de rôle avec la page (pas, pads, coupures…), l'étiquette est au-dessus
     · KNOB 1-4 = les 4 réglages affichés en bas de l'écran · SELECT = curseur / clic = action principale
   ===================================================================== */
window.VIEW=/editeur|editor|rythme|melodie|bibliotheque/.test(location.hash)?'ed':'fm1';
const SECH={rythme:'rythme',melodie:'melodie',sons:'bibliotheque'};
const M={sty:0,eid:null,bi:0,nr:-1,sub:{},grp:'HOME',allRows:false,nt:0,np:0,lastRow:0,lastTrk:0,cf:{},page:'home',pt:8,oct:0,cur:0,deg:0,held:new Map(),fxb:0,lib:{lvl:0,i:0,f:''},mel:0,toast:null,taps:[],last:{},lowDown:new Set(),upDown:new Set()};
const COL={bg:'#101215',p:'#1b1e22',p2:'#262a30',l:'#3a4048',t:'#eef1f5',m:'#939ba5',a:'#ff8a1f',y:'#ffd23f',c:'#35e3c4',d:'#0a0b0d',r:'#ff4646',pk:'#ffb3dc',ad:'#b9610f'};
const FONT='"Arial Narrow","Roboto Condensed","Helvetica Neue",Arial,sans-serif';
const gkL={mf:'COUPE-HAUT',hp:'COUPE-BAS',xd:'ÉCHO',xr:'REVERB',mq:'RÉSO',xs:'DISTO'};
const FTY=['lowpass','highpass','bandpass'],FTN=['PASSE-BAS','PASSE-HAUT','PASSE-BANDE'];
const STY=Object.keys(STYLES),SCN=Object.keys(SCALES),LEN=[.25,.5,.75,1,1.5,2,3,4,6,8,12,16],WAV=['sawtooth','square','triangle'];
const WHITE=[0,2,4,5,7,9,11,12,14,16,17,19,21,23,24,26],BLACK=[1,3,6,8,10,13,15,18,20,22,25],BPOS=[0,1,3,4,5,7,8,10,11,12,14]; /* touche noire k : après la touche blanche BPOS[k] */
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v)),pct=v=>Math.round(v*100)+'%';
const isD=()=>M.pt<8,ti=()=>M.pt-8,trk=()=>S.tracks[M.pt-8],rowO=()=>S.rows[M.pt];
const nameOf=p=>p<8?ROWS[p]:S.tracks[p-8].name;
const degName=d=>{const sc=SCALES[S.scale],L=sc.length;return NOTES[(S.root+sc[((d%L)+L)%L])%12];};
const now=()=>ctx?ctx.currentTime:0;
function toast(t){M.toast={t,u:performance.now()+1400};}
function say(t){toast(t);msg(t);}
function setPt(p){M.eid=null;const sc=PAGES[M.page]&&PAGES[M.page].sc;if(sc==='r')p=(p%8+8)%8;else if(sc==='t')p=8+((p-8)%6+6)%6;M.pt=(p%14+14)%14;if(M.pt<8)M.lastRow=M.pt;else M.lastTrk=M.pt-8;if(!isD())S.sel=ti();M.cur=Math.min(M.cur,15);toast((isD()?'BATTERIE · ':'MÉLODIE · ')+nameOf(M.pt));}
const fmtLen=l=>l<1?['','¼','½','¾'][l*4]+' PAS':String(l).replace('.',',')+' PAS';
const ab=(s,n)=>{s=String(s).toUpperCase();return s.length>n?s.slice(0,n-1)+'.':s;};

/* ---------- Réglages des knobs ---------- */
const K=(l,get,set,mn,mx,st,fmt)=>({l,get,set,mn,mx,st:st||1,fmt:fmt||String});
const NA=K('',()=>0,()=>{},0,0,1,()=>'');
const pk=(l,o,k,mx)=>K(l,()=>+o()[k]||0,v=>{o()[k]=v;applyParams();},0,mx||1,.01,pct);
const gk=(l,k)=>Object.assign(K(l,()=>+S[k]||0,v=>{S[k]=v;const e=$('#'+k);if(e)e.value=v;amT(k);applyParams();},0,1,.01,pct),{g:1});
function kTurn(p,dl){if(p.mx===p.mn||!dl)return;const v=clamp(Math.round((p.get()+dl*p.st)/p.st)*p.st,p.mn,p.mx);if(Math.abs(v-p.get())>1e-9){p.set(+v.toFixed(4));save();}}
function kAbs(p,n){if(p.mx===p.mn)return;const v=clamp(p.mn+Math.round((p.mx-p.mn)*n/p.st)*p.st,p.mn,p.mx);if(Math.abs(v-p.get())>1e-9){p.set(+v.toFixed(4));save();}}
// Filtre DJ : centre = ouvert · à gauche le son s'étouffe (coupe-haut) · à droite les basses partent (coupe-bas)
const djK=()=>Object.assign(K('FILTRE DJ',()=>(S.hp||0)>0?+(S.hp).toFixed(2):+((S.mf==null?1:S.mf)-1).toFixed(2),v=>{S.mf=v<0?1+v:1;S.hp=v>0?v:0;
  ['mf','hp'].forEach(k=>{const e=$('#'+k);if(e)e.value=S[k];amT(k);});applyParams();},-1,1,.02,v=>Math.abs(v)<.01?'OUVERT':v<0?'◀ '+pct(-v):pct(v)+' ▶'),{g:1});
const bpmK=()=>K('BPM',()=>S.bpm,v=>{S.bpm=v;applyParams();},60,220,1);
const swK=()=>K('SWING',()=>S.swing||0,v=>{S.swing=v;},0,.6,.01,pct);
const volK=()=>isD()?K('VOLUME',()=>rowO().vol,v=>{rowO().vol=v;applyParams();},0,1,.01,pct):pk('VOLUME',trk,'vol');
const arK=()=>{const o=isD()?rowO():trk();return K('AUTO',()=>Math.max(0,AUS.indexOf(o.ar||0)),v=>{o.ar=AUS[v];},0,AUS.length-1,1,v=>AUS[v]?AUS[v]+' MES':'OFF');};
const lpK=o=>K('LOOP',()=>Math.max(0,LPS.indexOf(o.lp||0)),v=>{o.lp=LPS[v];LP={};},0,LPS.length-1,1,v=>LPS[v]?LPS[v]+' PAS':'OFF');
const rlK=o=>K('ROLL',()=>Math.max(0,RLS.indexOf(o.rl||1)),v=>{o.rl=RLS[v];},0,RLS.length-1,1,v=>RLS[v]>1?'×'+RLS[v]:'OFF');
const progK=()=>K('ACCORDS',()=>S.prog||0,v=>{$('#prog').value=v;$('#prog').oninput();},0,(PROGS[S.scale]||[]).length,1,v=>v?'SUITE '+v:'OFF');
const styK=()=>K('STYLE',()=>Math.max(0,STY.indexOf(S.style)),v=>{setStyle(STY[v]);styleSc();},0,STY.length-1,1,v=>ab(STY[v],9));
const stp=()=>trk().steps[M.cur];
function audition(d){if(!playing&&ctx&&!isD()){ctx.resume();note(ti(),d,ctx.currentTime,.2,0,0);}}

/* ---------- Pages ----------
   Chaque bouton a une ou plusieurs pages : appuie encore sur le même bouton pour passer à la suivante.
   Partout : les touches du HAUT jouent la piste, les 16 du BAS changent de rôle (étiquette au-dessus),
   KNOB 1-4 = réglages affichés en bas de l'écran, SELECT = choisir · clic SELECT = action principale. */
const GRP={HOME:['home','hryt','hmel','hfx'],SEQ:['ryt','mel'],ARP:['gsty','gvis','gryt','gmel'],FX:['fxr','fxa','fxb','fxd','fxp'],SCL:['scl'],ENV:['env','flt','lfo','osc'],LFO:['mix','lrr'],EDIT:['son','sonr','sedit','emidi'],GLO:['glo','midi'],SAVE:['bank','bkr','bkm','nom']};
const grpOf=p=>Object.keys(GRP).find(b=>GRP[b].includes(p));
const blank=n=>Array(n).fill('');
// Actions communes
const muteRow=r=>{S.rows[r].mute=!S.rows[r].mute;applyParams();save();say(ROWS[r]+(S.rows[r].mute?' coupé':' relancé'));};
const muteTrk=i=>{const t=S.tracks[i];t.mute=!t.mute;applyParams();save();say(t.name+(t.mute?' coupé':' relancé'));};
const muteGrp=k=>{S.gm[k]=S.gm[k]?0:1;applyParams();save();say((k==='r'?'Tout le rythme':'Toute la mélodie')+(S.gm[k]?' coupé':' relancé'));};
const soloK=k=>{solo(k);say(S.solo.length?'Solo : '+S.solo.length+' piste(s)':'Solos retirés');};
const noSolo=()=>{S.solo=[];applyParams();save();say('Solos retirés');};
const rndRhythm=()=>{randomRhythm();say('Nouveau rythme '+S.style+(playing?' à la prochaine mesure':''));};
const rndRow=r=>{randomRow(r);say('Nouveau motif sur '+ROWS[r]);};
const rndTrk=i=>{randomMel(i);delete LP['t'+i];say('Nouvelle mélodie sur '+S.tracks[i].name);};
const withSel=(i,fn)=>{const s=S.sel;S.sel=i;try{fn();}finally{S.sel=s;}};
const sndK=()=>{const tr=trk(),o=['','piano',...(tr.smp&&tr.smp!=='piano'?[tr.smp]:[])];
  return K('SON',()=>Math.max(0,o.indexOf(tr.smp||'')),v=>withSel(ti(),()=>setSmp(o[v])),0,o.length-1,1,v=>['SYNTHÉ','PIANO','SAMPLE'][v]);};
const melK=()=>K('MÉLODIE',()=>Math.min(M.mel,Math.max(0,S.mels.length-1)),v=>{M.mel=v;},0,Math.max(0,S.mels.length-1),1,v=>S.mels[v]?ab(S.mels[v].name,9):'—');
const loadMel=k=>{const m=S.mels[k];if(!m)return say('Aucune mélodie à cet emplacement');launchMel(k);};
// Ajouter à la banque : le rythme en cours (copie s'il y est déjà) et la mélodie d'une piste
function addRhythm(){const r=cur();if(r.tmp){r.tmp=false;renderBank();save();return say('Rythme « '+r.name+' » ajouté à la banque');}
  const c=JSON.parse(JSON.stringify(r));c.name=(r.name.replace(/ \d+$/,'')+' '+(S.rhythms.length+1)).slice(0,24);c.tmp=false;S.rhythms.push(c);renderBank();save();say('Rythme ajouté : « '+c.name+' » ('+S.rhythms.length+')');}
function addMel(){if(isD())setPt(8+M.lastTrk);saveMel();}
// Lancer une mélodie enregistrée : elle se charge sur sa piste d'origine
function launchMel(k){const m=S.mels[k];if(!m)return;const t=m.t!=null&&S.tracks[m.t]?m.t:isD()?M.lastTrk:ti();M.pt=8+t;M.lastTrk=t;S.sel=t;M.mel=k;
  withSel(t,()=>$('#mbank').querySelector(`[data-ml="${k}"]`)?.click());say('▶ Mélodie « '+m.name+' » sur '+S.tracks[t].name+(playing?'':' — PLAY pour l’écouter'));}
const saveMel=()=>{if(isD())return say('Choisis une piste mélodique');const n=S.mels.length;withSel(ti(),()=>$('#msave').onclick());if(S.mels.length>n){M.mel=S.mels.length-1;say('Mélodie « '+S.mels[M.mel].name+' » enregistrée');}};
const delMel=j=>{const m=S.mels[M.mel];if(!m)return say('Aucune mélodie enregistrée');if(!sure(j,'SUPPR. « '+ab(m.name,14)+' » ?'))return;S.mels.splice(M.mel,1);M.mel=Math.max(0,M.mel-1);save();say('Mélodie supprimée');};
function sure(j,t){const n=performance.now();if(M.cf[j]>n){M.cf[j]=0;return true;}M.cf[j]=n+2000;toast(t+' (APPUIE ENCORE)');return false;}
const cfL=(j,l)=>M.cf[j]>performance.now()?'SÛR ?':l;
// Pads de performance (toutes les pages FX) : à maintenir
const PERF={
  low:(j,dn)=>{const hold=(k,v)=>{if(dn){amT(k);if(latch)lvSet(k,LV[k]===v?0:v);else{lvUp();lvH=k;lvSet(k,v);}}else if(!latch&&lvH===k)lvUp();};
    if(j<4)return hold('lp',[1,2,4,8][j]);if(j<7)return hold('rl',[2,3,4][j-4]);if(j===7)return hold('rv',1);
    if(!dn)return;
    if(j<11){S.lt=['','r','t'][j-8];LP={};say('Effets sur : '+['tout','le rythme','la mélodie'][j-8]);}
    else if(j===11)$('#lat').onclick();else if(j===12)amRec();else if(j===13)$('#amOn').onclick();else if(j===14)$('#amClr').onclick();else $('#fl0').onclick();save();},
  lab:()=>({low:['LOOP 1','LOOP 2','LOOP 4','LOOP 8','ROLL ×2','ROLL ×3','ROLL ×4','REVERSE','TOUT','RYTHME','MÉLODIE','VERROU','REC FX','REJOUE','EFFACER','ZÉRO'],
    on:j=>j<4?LV.lp===[1,2,4,8][j]:j<7?LV.rl===[2,3,4][j-4]:j===7?!!LV.rv:j<11?(S.lt||'')===['','r','t'][j-8]:j===11?latch:j===12?arec:j===13?!!S.am.on&&Object.keys(S.am.d).length>0:false,
    rec:j=>j===12&&arec}),
  sel:d=>{const L=GRP.FX,k=(L.indexOf(M.page)+d+L.length)%L.length;goPage(L[k],1);},
  click:()=>$('#amOn').onclick()};
const PERFH=['Touches du bas','à maintenir : 1-4 Loop 1/2/4/8 · 5-7 Roll ×2/×3/×4 · 8 Reverse · 9-11 cible (tout / rythme / mélodie) · 12 verrou · 13 REC des mouvements · 14 rejoue · 15 effacer · 16 tout à zéro'];
// Loop / Roll / Sens / Reverse de la piste (pages MIX)
const tgt=()=>isD()&&M.allRows?S.rows:[isD()?rowO():trk()];
const MIXL={
  low:(j,dn)=>{const o=isD()?rowO():trk();
    if(j===14){if(isD()){fxR=M.allRows?-1:M.pt;dn?drevOn(1):drevOff(1);}else dn?revOn(1):revOff(1);return;}
    if(!dn)return;
    if(j<7){tgt().forEach(x=>x.lp=LPS[j]);LP={};say('Loop '+(LPS[j]?LPS[j]+' pas':'off')+' sur '+(isD()&&M.allRows?'toutes les lignes':nameOf(M.pt)));}
    else if(j>=8&&j<12){tgt().forEach(x=>x.rl=RLS[j-8]);say('Roll '+(RLS[j-8]>1?'×'+RLS[j-8]:'off')+' sur '+(isD()&&M.allRows?'toutes les lignes':nameOf(M.pt)));}
    else if(j===13&&!isD())bwToggle();
    else if(j===15&&isD()){M.allRows=!M.allRows;say(M.allRows?'Loop, Roll et Reverse sur toutes les lignes de batterie':'Loop, Roll et Reverse sur la ligne '+ROWS[M.pt]);}
    save();},
  lab:()=>{const o=isD()?rowO():trk();return{low:['LOOP OFF','LOOP 1','LOOP 2','LOOP 3','LOOP 4','LOOP 6','LOOP 8','','ROLL OFF','ROLL ×2','ROLL ×3','ROLL ×4','',isD()?'':'SENS ◀','REVERSE',isD()?'TOUTES LIGNES':''],
    on:j=>j<7?(o.lp||0)===LPS[j]:j>=8&&j<12?(o.rl||1)===RLS[j-8]:j===13?!isD()&&!!o.bw:j===14?(isD()?rvR>-2:rvI===ti()):j===15?isD()&&!!M.allRows:false};}};
const MIXH=['Touches du bas','1-7 loop (off, 1, 2, 3, 4, 6, 8 pas) · 9-12 roll · 14 sens de lecture ◀ · 15 reverse (à maintenir) · 16 toutes les lignes (batterie)'];
// Bibliothèque (pages SON)
const LIBP={
  ents:()=>{const L=M.lib;if(L.lvl===0){const fs=libFolders();return[{k:'int',l:isD()?'SON INTERNE':'SYNTHÉ INTERNE'},...(isD()?[]:[{k:'piano',l:'PIANO (SON DE BASE)'}]),...fs.map(f=>({k:'f',f,l:f,n:S.lib.filter(x=>x.f===f).length+(f==='Synth'&&!isD()?1:0)}))];}
    return[...(L.f==='Synth'&&!isD()?[{k:'piano',l:'Piano (son de base)'}]:[]),...S.lib.filter(x=>x.f===L.f).map(x=>({k:'s',id:x.id,l:x.name}))];},
  enter:()=>{const id=isD()?rowO().smp:trk().smp,l=S.lib.find(x=>x.id===id),E=()=>LIBP.ents();
    if(l){M.lib={lvl:1,f:l.f,i:0};M.lib.i=Math.max(0,E().findIndex(e=>e.id===id));}
    else{const f=isD()?ROWF[M.pt]:TYPF[trk().type];M.lib={lvl:0,f:'',i:0};M.lib.i=Math.max(0,E().findIndex(e=>id==='piano'?e.k==='piano':e.f===f));}},
  pick:e=>{if(!e)return;
    if(e.k==='int'){if(isD()){rowO().smp='';say(ROWS[M.pt]+' : son interne');}else withSel(ti(),()=>setSmp(''));save();}
    else if(e.k==='piano'){withSel(ti(),()=>setSmp('piano'));audition(M.deg||0);}
    else if(e.k==='f'){M.lib={lvl:1,f:e.f,i:0};if(!e.n)toast('DOSSIER VIDE — GLISSE DES SONS SUR LE FM-1');}
    else{if(isD()){rowO().smp=e.id;save();say('« '+e.l+' » sur '+ROWS[M.pt]);drum(M.pt,now()+.01,1);}else withSel(ti(),()=>setSmp(e.id));}},
  low:(j,dn)=>{if(!dn)return;const e=LIBP.ents()[M.lib.i];
    if(j===0){if(M.lib.lvl){const f=M.lib.f;M.lib={lvl:0,f:'',i:Math.max(0,LIBP.ents().findIndex(x=>x.f===f))};}}
    else if(j===1){if(e&&(e.k==='s'||e.k==='piano'))preview(e.k==='piano'?'piano':e.id);}
    else if(j===2)LIBP.pick(e);else if(j===3)LIBP.pick({k:'int'});else if(j===4&&!isD())LIBP.pick({k:'piano'});
    else if(j===6||j===7)addOwn(j===7);},
  lab:()=>({low:['◀ RETOUR','▶ ÉCOUTER','✓ CHOISIR',isD()?'INTERNE':'SYNTHÉ',isD()?'':'PIANO','','＋ MES SONS','＋ DOSSIER',...blank(8)],on:j=>j<(isD()?4:5)||j===6||j===7}),
  sel:d=>{const n=LIBP.ents().length;M.lib.i=clamp(M.lib.i+d,0,Math.max(0,n-1));const e=LIBP.ents()[M.lib.i];if(e&&e.k==='s')preview(e.id);},
  click:()=>LIBP.pick(LIBP.ents()[M.lib.i])};
const LIBH=['Touches du bas','1 retour · 2 écouter · 3 choisir · 4 son interne · 5 piano · 7 ＋ ajouter mes sons (dans le dossier ouvert) · 8 ＋ importer un dossier entier'];
// Ajouter mes propres sons depuis la vue FM-1 : fichiers ou dossier entier, rangés dans le dossier ouvert (sinon devinés d'après le nom)
function addOwn(dir){libCur=(M.grp==='EDIT'&&M.lib.lvl)?M.lib.f:'';const i=$(dir?'#dirIn':'#libIn');
  try{i.click();toast(dir?'CHOISIS UN DOSSIER DE SONS…':'CHOISIS TES FICHIERS AUDIO…'+(libCur?' → '+libCur.toUpperCase():''));}catch(e){say('Utilise la souris pour ouvrir le sélecteur de fichiers');}}
// Pas d'une piste (pages RYTHME / MÉLODIE)
const STEP={
  low:(j,dn)=>{
    if(isD()){if(!dn)return;const g=cur().g,r=M.pt;g[r][j]=g[r][j]?0:1;cur().tmp=false;M.cur=j;save();return;}
    const tr=trk();
    if(dn){M.held.set(j,false);M.cur=j;return;}
    const used=M.held.get(j);M.held.delete(j);if(used)return;
    const s=tr.steps[j];if(s.n>=0)tr.steps[j]={n:-1,a:0,s:0,o:0,l:1};else{tr.steps[j]={n:M.deg,a:0,s:0,o:0,l:1};audition(M.deg);}save();},
  lab:()=>{if(isD()){const g=cur().g[M.pt];return{col:()=>trkC(M.pt),low:g.map((_,j)=>String(j+1)),on:j=>g[j]>0,acc:j=>g[j]>1,play:true};}
    const tr=trk(),tl=tails(tr);return{col:()=>trkC(M.pt),low:tr.steps.map((s,j)=>s.n>=0?degName(s.n):String(j+1)),on:j=>tr.steps[j].n>=0,half:j=>tl[j]>=0,acc:j=>!!tr.steps[j].a,play:true};},
  sel:d=>{M.cur=(M.cur+d+16)%16;},
  click:()=>{if(isD()){const g=cur().g,r=M.pt,v=g[r][M.cur];g[r][M.cur]=v===0?1:v===1?2:0;cur().tmp=false;save();toast('PAS '+(M.cur+1)+' : '+['VIDE','NOTE','ACCENT'][g[r][M.cur]]);}
    else{STEP.low(M.cur,true);STEP.low(M.cur,false);}}};
const melSel=d=>setPt(8+((ti()+d)%6+6)%6),rowSel=d=>setPt(((M.pt+d)%8+8)%8);
const NAMEC=' ABCDEFGHIJKLMNOPQRSTUVWXYZÉÈ0123456789-·#';
const nObj=()=>M.nt?S.mels[M.mel]:(S.rhythms[M.nr]||cur());

const PAGES={
/* HOME : 1 général · 2 rythme · 3 mélodie */
home:{t:'LIVE',
  knobs:()=>[volK(),djK(),swK(),M.kp?koK():arK()],
  low:(j,dn)=>{if(!dn)return;if(j<8)muteRow(j);else if(j<14)muteTrk(j-8);else muteGrp(j===14?'r':'t');},
  lab:()=>({ico:j=>j<14?(j<8?ROWCAT[j]:TYPCAT[S.tracks[j-8].type]):'',col:j=>j<14?trkC(j):'',low:[...ROWS,...S.tracks.map(t=>t.name),'RYTHME','MÉLODIE'],on:j=>j<8?!S.rows[j].mute:j<14?!S.tracks[j-8].mute:!S.gm[j===14?'r':'t']}),
  sel:d=>setPt(M.pt+d),
  click:()=>soloK((isD()?'r':'t')+(isD()?M.pt:ti())),
  help:[['Touches du bas','couper / relancer : 1-8 batterie · 9-14 mélodie · 15 tout le rythme · 16 toute la mélodie'],['SELECT','choisir la piste · clic = solo'],['PRESETS','changer le son de la piste : les sons de son dossier (CLAP → les claps…)'],['OCT- / OCT+','couper / relancer tout le rythme / toute la mélodie'],['Knobs','volume · filtre DJ (gauche = étouffé, droite = sans basses) · swing · auto-random de la piste (mode JEU : octave du clavier)'],['HOME maintenu 2 s','🎲 nouvelle génération pour la piste choisie'],['ALGORITHM','à droite = mode JEU : les 27 touches jouent la piste choisie · à gauche = retour aux coupures']]},
hryt:{t:'LIVE RYTHME',sc:'r',
  knobs:()=>[volK(),arK(),styK(),M.kp?koK():K('RYTHME',()=>S.gm.r?0:1,v=>{S.gm.r=v?0:1;applyParams();},0,1,1,v=>v?'ACTIF':'COUPÉ')],
  low:(j,dn)=>{if(!dn)return;if(j<8)muteRow(j);else soloK('r'+(j-8));},
  lab:()=>({ico:j=>ROWCAT[j%8],col:j=>trkC(j%8),low:[...ROWS,...ROWS.map(r=>'SOLO '+r)],on:j=>j<8?!S.rows[j].mute:S.solo.includes('r'+(j-8))}),
  sel:rowSel,click:rndRhythm,
  help:[['Touches du bas','1-8 couper / relancer chaque son · 9-16 solo'],['SELECT','choisir la ligne · clic = 🎲 nouveau rythme dans le style'],['HOME maintenu 2 s','🎲 nouveau motif pour la ligne choisie'],['Knobs','volume et auto-random de la ligne · style · tout le rythme actif / coupé']]},
hmel:{t:'LIVE MÉLODIE',sc:'t',
  knobs:()=>[volK(),arK(),K('PITCH',()=>trk().po||0,v=>{trk().po=v;applyParams();},-3,3,1,v=>(v>0?'+':'')+v+' OCT'),M.kp?koK():sndK()],
  low:(j,dn)=>{if(!dn)return;if(j<6)muteTrk(j);else if(j===6)muteGrp('t');else if(j===7)rndTrk(ti());else if(j>=8&&j<14)soloK('t'+(j-8));else if(j===14)$('#mall').onclick();else if(j===15)noSolo();},
  lab:()=>({ico:j=>j<6?TYPCAT[S.tracks[j].type]:j>=8&&j<14?TYPCAT[S.tracks[j-8].type]:'',col:j=>j<6?trkC(8+j):j===7?trkC(M.pt):j>=8&&j<14?trkC(j):'',low:[...S.tracks.map(t=>t.name),'MÉLODIE','🎲 '+trk().name,...S.tracks.map(t=>'SOLO '+t.name),'🎲 TOUTES','SANS SOLO'],on:j=>j<6?!S.tracks[j].mute:j===6?!S.gm.t:j===7||j===14?true:j>=8&&j<14?S.solo.includes('t'+(j-8)):j===15}),
  sel:melSel,click:()=>rndTrk(ti()),
  help:[['SELECT','choisir BASSE, SYNTHÉ, BASSE 2, SYNTHÉ 2, NAPPE ou FX · clic = 🎲 nouvelle mélodie'],['Touches du bas','1-6 couper / relancer · 7 toute la mélodie · 8 🎲 nouvelle mélodie de la piste choisie · 9-14 solo · 15 🎲 toutes les pistes · 16 enlever les solos'],['HOME maintenu 2 s','🎲 nouvelle mélodie pour la piste choisie'],['Knobs','volume · auto-random · pitch · son (synthé / piano / sample)']]},
/* SEQ : 1 rythme seul · 2 mélodie seule */
ryt:Object.assign({t:'RYTHME',sc:'r',
  knobs:()=>[K('LIGNE',()=>M.pt,v=>{M.pt=v;M.lastRow=v;},0,7,1,v=>ROWS[v]),volK(),lpK(rowO()),rlK(rowO())],
  help:[['Touches du bas','les 16 pas de la ligne : appui = poser / enlever'],['Touches du haut','choisir la ligne (et la jouer) · avec REC, les coups s’écrivent'],['SELECT','choisir le pas · clic = vide → note → accent']]},{low:STEP.low,lab:STEP.lab,sel:STEP.sel,click:STEP.click}),
mel:Object.assign({t:'MÉLODIE',sc:'t',
  knobs:()=>[K('NOTE',()=>Math.max(0,stp().n),v=>{const s=stp();if(s.n<0){s.l=1;s.o=0;}s.n=v;M.deg=v;audition(v);},0,2*SL(),1,v=>stp().n<0?'—':degName(v)+(Math.floor(v/SL())+1)),
    K('DURÉE',()=>{const l=stp().l||1;let b=0;LEN.forEach((x,k)=>{if(Math.abs(x-l)<Math.abs(LEN[b]-l))b=k;});return b;},v=>{stp().l=Math.min(LEN[v],16-M.cur-(stp().o||0));},0,LEN.length-1,1,v=>fmtLen(LEN[v])),
    K('ACC · SLIDE',()=>(stp().a?1:0)+(stp().s?2:0),v=>{stp().a=v&1;stp().s=v>>1;},0,3,1,v=>['—','ACCENT','SLIDE','ACC+SL'][v]),
    K('DÉCALAGE',()=>Math.round((stp().o||0)*4),v=>{const s=stp();s.o=v/4;s.l=Math.min(s.l||1,16-M.cur-s.o);},0,3,1,v=>v?'+'+['','¼','½','¾'][v]+' PAS':'SUR LE PAS')],
  help:[['Touches du bas','les 16 pas : appui = poser / enlever · maintiens un pas + touche du haut = sa note'],['Knobs','note · durée · accent/slide · décalage (contre‑temps) du pas'],['PRESETS','changer de piste mélodique']]},{low:STEP.low,lab:STEP.lab,sel:STEP.sel,click:STEP.click}),
/* ARP = GEN : même logique pour le rythme et la mélodie (touches 1-8 au même endroit) */
gryt:{t:'GEN RYTHME',sc:'r',
  knobs:()=>[styK(),K('AUTO RYT.',()=>Math.max(0,AUS.indexOf(S.auto||0)),v=>{S.auto=AUS[v];},0,AUS.length-1,1,v=>AUS[v]?AUS[v]+' MES':'OFF'),arK(),swK()],
  low:(j,dn)=>{if(!dn)return;
    if(j===0)rndRhythm();else if(j===1){$('#vary').onclick();say('Rythme varié');}else if(j===2)rndRow(M.pt);
    else if(j===3){cur().g[M.pt]=Array(16).fill(0);cur().tmp=false;save();say(ROWS[M.pt]+' vidée');}
    else if(j===4){$('#clr').onclick();say('Rythme vidé');}else if(j===5){$('#keep').onclick();say('Rythme gardé dans la banque');}
    else if(j===6){$('#dup').onclick();say('Rythme dupliqué');}
    else if(j===7){if(S.rhythms.length<2)return say('Il faut garder au moins un rythme');if(!sure(7,'SUPPR. LE RYTHME '+(S.cur+1)+' ?'))return;$('#del').onclick();say('Rythme supprimé');}},
  lab:()=>({low:['🎲 RYTHME','VARIER','🎲 LIGNE','VIDER LIGNE','VIDER TOUT','GARDER','DUPLIQUER',cfL(7,'SUPPR.'),...blank(8)],on:j=>j<8,q:j=>M.cf[j]>performance.now()}),
  sel:d=>{const k=(STY.indexOf(S.style)+d+STY.length)%STY.length;setStyle(STY[k]);styleSc();save();},click:rndRhythm,
  help:[['Touches du bas','1 🎲 nouveau rythme · 2 varier · 3 🎲 la ligne · 4 vider la ligne · 5 vider tout · 6 garder · 7 dupliquer · 8 supprimer (2 appuis)'],['SELECT','style (tempo et gamme suivent) · clic = 🎲'],['Knobs','style · auto-random du rythme · auto de la ligne · swing']]},
gmel:{t:'GEN MÉLODIE',sc:'t',
  knobs:()=>[progK(),arK(),melK(),K('GAMME',()=>SCN.indexOf(S.scale),v=>{$('#scale').value=SCN[v];$('#scale').oninput();},0,SCN.length-1,1,v=>ab(SCN[v],10))],
  low:(j,dn)=>{if(!dn)return;
    if(j===0)rndTrk(ti());else if(j===1){varyMel(ti());say('Mélodie de '+trk().name+' variée');}else if(j===2)$('#mall').onclick();
    else if(j===3){trk().steps=stepsNew();save();say(trk().name+' vidée');}
    else if(j===4){if(!sure(4,'VIDER LES 6 PISTES ?'))return;S.tracks.forEach(t=>t.steps=stepsNew());save();say('Toutes les mélodies vidées');}
    else if(j===5)saveMel();else if(j===6)loadMel(M.mel);else if(j===7)delMel(7);},
  lab:()=>({low:['🎲 PISTE','VARIER','🎲 TOUTES','VIDER PISTE',cfL(4,'VIDER TOUT'),'ENREGIST.','CHARGER',cfL(7,'SUPPR.'),...blank(8)],on:j=>j<8,q:j=>M.cf[j]>performance.now()}),
  sel:melSel,click:()=>rndTrk(ti()),
  help:[['Touches du bas','1 🎲 nouvelle mélodie · 2 varier · 3 🎲 toutes les pistes · 4 vider la piste · 5 vider tout · 6 enregistrer · 7 charger la mélodie choisie · 8 la supprimer'],['SELECT','choisir la piste · clic = 🎲'],['Knobs','accords · auto-random de la piste · mélodie de la banque · gamme']]},
/* FX : 4 pages comme des pédales, les 16 pads de performance partout */
fxa:Object.assign({t:'FX FILTRE',knobs:()=>[gk('COUPE-HAUT','mf'),gk('COUPE-BAS','hp'),gk('RÉSO','mq'),K('CIBLE',()=>Math.max(0,['','r','t'].indexOf(S.lt||'')),v=>{S.lt=['','r','t'][v];LP={};},0,2,1,v=>['TOUT','RYTHME','MÉLODIE'][v])],
  help:[['Filtre général','coupe-haut (le son s’étouffe) · coupe-bas (les basses partent) · résonance · cible des pads'],PERFH,['SELECT','effet suivant · clic = rejoue / pause des mouvements']]},PERF),
fxb:Object.assign({t:'FX ESPACE',knobs:()=>[gk('ÉCHO','xd'),gk('REVERB','xr'),gk('DISTO','xs'),K('BOUCLE REC',()=>Math.max(0,[1,2,4,8].indexOf(S.am.n||4)),v=>{S.am.n=[1,2,4,8][v];amUI();},0,3,1,v=>[1,2,4,8][v]+' MES')],
  help:[['Effets généraux','écho calé sur le tempo · reverb · disto · longueur de la boucle REC'],PERFH]},PERF),
fxd:Object.assign({t:'FX BATTERIE',knobs:()=>[pk('VOLUME',()=>S.dfx,'vol'),pk('DISTO',()=>S.dfx,'dist'),pk('DELAY',()=>S.dfx,'dly'),pk('REVERB',()=>S.dfx,'rev')],
  help:[['Toute la batterie','volume · disto · delay · reverb'],PERFH]},PERF),
fxp:Object.assign({t:'FX PERF',knobs:()=>[K('CIBLE',()=>Math.max(0,['','r','t'].indexOf(S.lt||'')),v=>{S.lt=['','r','t'][v];LP={};},0,2,1,v=>['TOUT','RYTHME','MÉLODIE'][v]),
    K('BOUCLE REC',()=>Math.max(0,[1,2,4,8].indexOf(S.am.n||4)),v=>{S.am.n=[1,2,4,8][v];amUI();},0,3,1,v=>[1,2,4,8][v]+' MES'),K('VERROU',()=>latch?1:0,v=>{if(!!v!==latch)$('#lat').onclick();},0,1,1,v=>v?'ON':'OFF'),NA],
  help:[PERFH,['REC','enregistre les mouvements des pads et des knobs, ils rejouent en boucle'],['Knobs','cible des pads · longueur de la boucle · verrou (un appui garde l’effet)']]},PERF),
/* SCL */
scl:{t:'GAMME',
  knobs:()=>[K('TONIQUE',()=>S.root,v=>{$('#root').value=v;$('#root').oninput();},0,11,1,v=>NOTES[v].toUpperCase()),
    K('GAMME',()=>SCN.indexOf(S.scale),v=>{$('#scale').value=SCN[v];$('#scale').oninput();},0,SCN.length-1,1,v=>ab(SCN[v],10)),progK(),
    isD()?K('OCT CLAV.',()=>M.oct,v=>{M.oct=v;},-2,2,1,v=>(v>0?'+':'')+v):K('PITCH',()=>trk().po||0,v=>{trk().po=v;applyParams();},-3,3,1,v=>(v>0?'+':'')+v+' OCT')],
  low:(j,dn)=>{if(!dn||j>11)return;$('#root').value=j;$('#root').oninput();say('Tonique : '+NOTES[j]);},
  lab:()=>({low:[...NOTES.map(n=>n.toUpperCase()),...blank(4)],on:j=>j===S.root}),
  sel:d=>{const k=(SCN.indexOf(S.scale)+d+SCN.length)%SCN.length;$('#scale').value=SCN[k];$('#scale').oninput();},
  click:()=>{$('#prog').value=S.prog?0:1;$('#prog').oninput();},
  help:[['Touches du bas','1-12 = tonique (Do à Si)'],['SELECT','gamme (les notes posées sont recalées) · clic = accords on / off'],['Knobs','tonique · gamme · suite d’accords · pitch de la piste']]},
/* ENV : 1 enveloppe · 2 filtre · 3 LFO · 4 oscillateur (touches du bas communes : onde, octave, synthé/piano, type de filtre) */
env:{t:'ENV',sc:'t',
  knobs:()=>[pk('ATTAQUE',trk,'att'),pk('DECAY',trk,'dec'),pk('ENV → FILTRE',trk,'env'),pk('GLIDE',trk,'gl')],
  low:(j,dn)=>{if(!dn)return;const tr=trk();
    if(j<3){tr.wave=WAV[j];say('Onde : '+['scie','carré','triangle'][j]);}else if(j>=4&&j<10){tr.oct=j-3;say('Octave '+tr.oct);}
    else if(j===10)withSel(ti(),()=>setSmp(''));else if(j===11)withSel(ti(),()=>setSmp('piano'));
    else if(j>=13){tr.ftype=FTY[j-13];say('Filtre '+FTN[j-13]);}applyParams();save();},
  lab:()=>({col:()=>trkC(M.pt),low:['SCIE','CARRÉ','TRI','','OCT 1','OCT 2','OCT 3','OCT 4','OCT 5','OCT 6','SYNTHÉ','PIANO','','PASSE-BAS','PASSE-HAUT','BANDE'],
    on:j=>{const t=trk();return j<3?t.wave===WAV[j]:j>=4&&j<10?t.oct===j-3:j===10?!t.smp:j===11?t.smp==='piano':j>=13?(t.ftype||'lowpass')===FTY[j-13]:false;}}),
  sel:melSel,click:()=>audition(M.deg||0),
  help:[['Knobs','attaque (le son arrive doucement) · decay · enveloppe vers le filtre · glide (les notes glissent)'],['Touches du bas','1-3 onde · 5-10 octave · 11 synthé · 12 piano · 14-16 type de filtre'],['SELECT','choisir la piste · clic = écouter']]},
flt:{t:'FILTRE',sc:'t',knobs:()=>[pk('CUTOFF',trk,'cut'),pk('RÉSO',trk,'res'),K('TYPE',()=>Math.max(0,FTY.indexOf(trk().ftype||'lowpass')),v=>{trk().ftype=FTY[v];applyParams();},0,2,1,v=>FTN[v]),pk('ENV → FILTRE',trk,'env')],
  sel:melSel,click:()=>audition(M.deg||0),
  help:[['Knobs','coupure · résonance · type : passe-bas (étouffe), passe-haut (enlève les basses), passe-bande (son de radio) · enveloppe'],['Touches du bas','1-3 onde · 5-10 octave · 11 synthé · 12 piano · 14-16 type de filtre']]},
lfo:{t:'LFO',sc:'t',knobs:()=>[K('VITESSE',()=>trk().lfr==null?.3:trk().lfr,v=>{trk().lfr=v;applyParams();},0,1,trk().lfs?.25:.01,v=>trk().lfs?['1 MES','1/2','1/4','1/8','1/16'][Math.round(v*4)]:lfoHz(Object.assign({},trk(),{lfr:v})).toFixed(1)+' HZ'),
    pk('PROFONDEUR',trk,'lfd'),K('FORME',()=>trk().lfw||0,v=>{trk().lfw=v;applyParams();},0,3,1,v=>['SINUS','TRIANGLE','CARRÉ','SCIE'][v]),K('CALAGE',()=>trk().lfs?1:0,v=>{trk().lfs=v;applyParams();},0,1,1,v=>v?'TEMPO':'LIBRE')],
  sel:melSel,click:()=>audition(M.deg||0),
  help:[['LFO','fait bouger la coupure du filtre en boucle : wobble, effet de respiration'],['Knobs','vitesse (en Hz, ou en mesure si calé sur le tempo) · profondeur · forme · libre / calé sur le tempo']]},
osc:{t:'OSC',sc:'t',
  knobs:()=>[K('ONDE',()=>Math.max(0,WAV.indexOf(trk().wave)),v=>{trk().wave=WAV[v];applyParams();},0,2,1,v=>['SCIE','CARRÉ','TRI'][v]),K('OCTAVE',()=>trk().oct,v=>{trk().oct=v;},1,6,1),
    K('PITCH',()=>trk().po||0,v=>{trk().po=v;applyParams();},-3,3,1,v=>(v>0?'+':'')+v+' OCT'),sndK()],
  sel:melSel,click:()=>audition(M.deg||0),
  help:[['Oscilloscope','le vrai son de la piste, en direct'],['Knobs','onde · octave · pitch · son : synthé, piano ou sample'],['Touches du bas','1-3 onde · 5-10 octave · 11 synthé · 12 piano · 14-16 type de filtre']]},
/* LFO = MIX : 1 niveaux et effets de la piste · 2 loop / roll / sens */
mix:Object.assign({t:'MIX',
  knobs:()=>isD()?[volK(),arK(),pk('VOL BATT.',()=>S.dfx,'vol'),pk('REV BATT.',()=>S.dfx,'rev')]:[pk('VOLUME',trk,'vol'),pk('DISTO',trk,'dist'),pk('DELAY',trk,'dly'),pk('REVERB',trk,'rev')],
  sel:d=>setPt(M.pt+d),click:()=>{},
  help:[['Knobs','mélodie : volume, disto, delay, reverb de la piste · batterie : volume et auto de la ligne, volume et reverb de la batterie'],MIXH,['SELECT','choisir la piste']]},MIXL),
lrr:Object.assign({t:'LOOP ROLL',
  knobs:()=>{const o=isD()?rowO():trk();return[lpK(o),rlK(o),isD()?K('CIBLE',()=>M.allRows?1:0,v=>{M.allRows=!!v;},0,1,1,v=>v?'TOUTES':'LIGNE'):K('SENS',()=>o.bw?1:0,v=>{if(!!v!==!!o.bw)bwToggle();},0,1,1,v=>v?'◀ INVERSE':'▶ NORMAL'),arK()];},
  sel:d=>setPt(M.pt+d),click:()=>{},
  help:[['Knobs','loop · roll · sens de lecture (mélodie) ou cible toutes les lignes (batterie) · auto-random'],MIXH]},MIXL),
/* EDIT = SON : 1 bibliothèque · 2 réglages du son */
son:Object.assign({t:'SON',knobs:()=>isD()?[volK(),NA,NA,NA]:[sndK(),K('NOTE SON',()=>trk().sn||0,v=>{trk().sn=v;},0,11,1,v=>NOTES[v].toUpperCase()),K('DÉBUT',()=>trk().ss||0,v=>{trk().ss=v;},0,.95,.01,pct),K('OCTAVE',()=>trk().oct,v=>{trk().oct=v;},1,6,1)],
  help:[['SELECT','parcourir la bibliothèque (pré-écoute) · clic = ouvrir le dossier / choisir'],LIBH,['Ajouter des sons','glisse tes fichiers sur le FM-1 virtuel : ils vont dans le dossier ouvert']]},LIBP),
sonr:Object.assign({t:'SON RÉGLAGES',knobs:()=>isD()?[volK(),arK(),NA,NA]:[K('NOTE SON',()=>trk().sn||0,v=>{trk().sn=v;},0,11,1,v=>NOTES[v].toUpperCase()),K('DÉBUT',()=>trk().ss||0,v=>{trk().ss=v;},0,.95,.01,pct),
    K('ONDE',()=>Math.max(0,WAV.indexOf(trk().wave)),v=>{trk().wave=WAV[v];applyParams();},0,2,1,v=>['SCIE','CARRÉ','TRI'][v]),K('OCTAVE',()=>trk().oct,v=>{trk().oct=v;},1,6,1)],
  help:[['Knobs','note à laquelle ton son a été enregistré · début dans le fichier · onde · octave'],LIBH]},LIBP),
/* GLO : 1 tempo · 2 MIDI */
glo:{t:'TEMPO',
  knobs:()=>[bpmK(),swK(),styK(),K('AUTO RYT.',()=>Math.max(0,AUS.indexOf(S.auto||0)),v=>{S.auto=AUS[v];},0,AUS.length-1,1,v=>AUS[v]?AUS[v]+' MES':'OFF')],
  low:(j,dn)=>{if(!dn)return;if(j===0)PAGES.glo.click();else if(j>=2&&j<2+ALGO.length){S.algo=ALGO[j-2][0];save();say('ALGORITHM = '+ALGO[j-2][1].toLowerCase()+' : '+ALGO[j-2][2]);}},
  lab:()=>({low:['TAP TEMPO','',...ALGO.map(a=>'ALGO '+a[1]),...blank(8)],on:j=>j===0||j>=2&&j<2+ALGO.length&&(S.algo||'pages')===ALGO[j-2][0]}),
  sel:d=>{S.bpm=clamp(S.bpm+d,60,220);applyParams();save();},
  click:()=>{const t=performance.now();M.taps=M.taps.filter(x=>t-x<2500);M.taps.push(t);
    if(M.taps.length>1){const iv=(M.taps[M.taps.length-1]-M.taps[0])/(M.taps.length-1);S.bpm=clamp(Math.round(60000/iv),60,220);applyParams();save();}toast('TAP · '+S.bpm+' BPM');},
  help:[['Knobs','BPM · swing · style · auto-random du rythme'],['SELECT','BPM ±1 · clic ou touche 1 = tap tempo'],['Touches 3-8','rôle du knob ALGORITHM : pages (défaut), BPM, filtre DJ, volume de la piste, rythmes de la banque, swing']]},
midi:{t:'MIDI',
  knobs:()=>[chK(),outK(),clkK(),K('ENVOI HORL.',()=>S.mclk?1:0,v=>{S.mclk=v;},0,1,1,v=>v?'ON':'OFF')],
  low:(j,dn)=>{if(!dn)return;if(j===0)midiConnect();else if(j===1){S.mclk=S.mclk?0:1;save();say(S.mclk?'Horloge MIDI envoyée au FM-1':'Horloge MIDI coupée');}
    else if(j===2){midiPanic();say('Notes du FM-1 coupées');}else if(j===3)midiTest();else if(j===4)learnStart();},
  lab:()=>({low:['CONNECTER','HORLOGE','STOP NOTES','TESTER','CALER',...blank(11)],on:j=>j===0?!!MIDI.out:j===1?!!S.mclk:j<5}),
  sel:d=>setPt(M.pt+d),click:()=>midiConnect(),
  help:[['Knobs','canal et sortie de la piste (interne / FM‑1 / les deux) · horloge MIDI'],['Touches du bas','1 connecter le FM‑1 · 2 horloge · 3 stop notes · 4 tester le son · 5 caler le clavier du FM‑1']]},
/* SAVE = BANQUE : 1 menu (ajouter / lancer / supprimer) · 2 rythmes · 3 mélodies · 4 nom */
bank:{t:'BANQUE',
  ents:()=>[{k:'addR',l:'＋ AJOUTER LE RYTHME ACTUEL'},{k:'addM',l:'＋ AJOUTER LA MÉLODIE ('+(isD()?S.tracks[M.lastTrk]:trk()).name+')'},
    {k:'sep',l:'RYTHMES ('+S.rhythms.length+')'},...S.rhythms.map((r,i)=>({k:'r',i,l:(i+1)+' · '+r.name})),
    {k:'sep',l:'MÉLODIES ('+S.mels.length+')'},...(S.mels.length?S.mels.map((m,i)=>({k:'m',i,l:(i+1)+' · '+m.name})):[{k:'sep',l:'   aucune : ＋ AJOUTER LA MÉLODIE'}])],
  act:e=>{if(!e)return;if(e.k==='addR')addRhythm();else if(e.k==='addM')addMel();else if(e.k==='r'){go(S.rhythms[e.i]);say('▶ Rythme « '+S.rhythms[e.i].name+' »'+(playing?' à la prochaine mesure':' — PLAY pour l’écouter'));}else if(e.k==='m')launchMel(e.i);},
  low:(j,dn)=>{if(!dn)return;const E=PAGES.bank.ents(),e=E[M.bi];
    if(j===0)addRhythm();else if(j===1)addMel();else if(j===2)PAGES.bank.act(e);
    else if(j===3){if(!e||(e.k!=='r'&&e.k!=='m'))return say('Choisis un rythme ou une mélodie avec SELECT');
      if(e.k==='r'){if(S.rhythms.length<2)return say('Il faut garder au moins un rythme');if(!sure(3,'SUPPR. LE RYTHME '+(e.i+1)+' ?'))return;go(S.rhythms[e.i]);if(queued)applyR(queued);$('#del').onclick();say('Rythme supprimé');}
      else{M.mel=e.i;delMel(3);}M.bi=Math.min(M.bi,PAGES.bank.ents().length-1);}
    else if(j===4){if(!e||(e.k!=='r'&&e.k!=='m'))return say('Choisis un rythme ou une mélodie avec SELECT');M.nt=e.k==='m'?1:0;if(e.k==='m')M.mel=e.i;else M.nr=e.i;M.np=0;goPage('nom');}
    else if(j===5){$('#dup').onclick();say('Rythme dupliqué');}},
  lab:()=>({low:['＋ RYTHME','＋ MÉLODIE','▶ LANCER',cfL(3,'SUPPR.'),'RENOMMER','DUPLIQUER',...blank(10)],on:j=>j<6,q:j=>M.cf[j]>performance.now()}),
  sel:d=>{const E=PAGES.bank.ents();let i=M.bi;for(let k=0;k<E.length;k++){i=clamp(i+d,0,E.length-1);if(E[i].k!=='sep')break;}if(E[i].k!=='sep')M.bi=i;},
  click:()=>PAGES.bank.act(PAGES.bank.ents()[M.bi]),
  knobs:()=>[K('RYTHME',()=>queued?S.rhythms.indexOf(queued):S.cur,v=>{go(S.rhythms[v]);},0,S.rhythms.length-1,1,v=>String(v+1)),melK(),
    K('AUTO RYT.',()=>Math.max(0,AUS.indexOf(S.auto||0)),v=>{S.auto=AUS[v];},0,AUS.length-1,1,v=>AUS[v]?AUS[v]+' MES':'OFF'),NA],
  help:[['SELECT','parcourir la liste · clic = ajouter / lancer l’élément choisi'],['Touches du bas','1 ＋ ajouter le rythme actuel · 2 ＋ ajouter la mélodie de la piste · 3 ▶ lancer · 4 supprimer (2 appuis) · 5 renommer · 6 dupliquer le rythme'],['Lancer','un rythme part à la mesure suivante · une mélodie se charge sur sa piste d’origine']]},
bkr:{t:'RYTHMES',
  knobs:()=>[K('RYTHME',()=>queued?S.rhythms.indexOf(queued):S.cur,v=>{go(S.rhythms[v]);},0,S.rhythms.length-1,1,v=>String(v+1)),
    K('AUTO RYT.',()=>Math.max(0,AUS.indexOf(S.auto||0)),v=>{S.auto=AUS[v];},0,AUS.length-1,1,v=>AUS[v]?AUS[v]+' MES':'OFF'),NA,NA],
  low:(j,dn)=>{if(!dn)return;if(S.rhythms[j]){go(S.rhythms[j]);toast('RYTHME '+(j+1)+(playing?' À LA MESURE':''));}},
  lab:()=>({low:Array.from({length:16},(_,j)=>S.rhythms[j]?ab(S.rhythms[j].name.replace(/^.*· /,''),8):''),on:j=>j===S.cur,q:j=>queued&&S.rhythms[j]===queued}),
  sel:d=>turn('ALGORITHM',d),click:()=>{$('#keep').onclick();say('Rythme '+(S.cur+1)+' gardé dans la banque');},
  help:[['Touches du bas','les 16 rythmes : appui = jouer (à la mesure suivante)'],['SELECT','rythme suivant · clic = garder le rythme (les 🎲 tirés au hasard sont provisoires)']]},
bkm:{t:'MÉLODIES',sc:'t',
  knobs:()=>[melK(),NA,NA,NA],
  low:(j,dn)=>{if(dn)loadMel(j);},
  lab:()=>({low:Array.from({length:16},(_,j)=>S.mels[j]?ab(S.mels[j].name,8):''),on:j=>j===M.mel&&!!S.mels[j]}),
  sel:d=>{if(S.mels.length)M.mel=(M.mel+d+S.mels.length)%S.mels.length;},click:saveMel,
  help:[['Touches du bas','les mélodies enregistrées : appui = la lancer sur sa piste d’origine'],['SELECT','choisir · clic = enregistrer la mélodie de la piste'],['Supprimer','GEN MÉLODIE, touche 8']]},
nom:{t:'NOM',
  knobs:()=>[K('CIBLE',()=>M.nt||0,v=>{M.nt=v;M.np=0;M.nr=S.cur;},0,1,1,v=>v?'MÉLODIE':'RYTHME'),K('POSITION',()=>M.np||0,v=>{M.np=v;},0,23,1,v=>String(v+1)),
    K('LETTRE',()=>{const o=nObj(),c=o?(o.name[M.np||0]||' ').toUpperCase():' ';return Math.max(0,NAMEC.indexOf(c));},v=>{const o=nObj();if(!o)return;const a=[...o.name.padEnd((M.np||0)+1,' ')];a[M.np||0]=NAMEC[v];o.name=a.join('').trimEnd().slice(0,24)||' ';if(!M.nt)o.tmp=false;},0,NAMEC.length-1,1,v=>NAMEC[v]===' '?'ESPACE':NAMEC[v]),NA],
  low:(j,dn)=>{if(!dn)return;const o=nObj();if(!o)return;const p=M.np||0;
    if(j===0)M.np=Math.max(0,p-1);else if(j===1)M.np=Math.min(23,p+1);
    else if(j===2){o.name=(o.name.slice(0,p)+' '+o.name.slice(p)).slice(0,24);}
    else if(j===3){o.name=(o.name.slice(0,p)+o.name.slice(p+1))||' ';}
    if(!M.nt)o.tmp=false;save();},
  lab:()=>({low:['◀','▶','INSÉRER','EFFACER',...blank(12)],on:j=>j<4}),
  sel:d=>kTurn(PAGES.nom.knobs()[2],d),click:()=>{M.np=Math.min(23,(M.np||0)+1);},
  help:[['Renommer','KNOB1 rythme ou mélodie (celle choisie dans MÉLODIES) · KNOB2 position · KNOB3 ou SELECT lettre · clic = lettre suivante'],['Touches du bas','1 ◀ · 2 ▶ · 3 insérer un espace · 4 effacer la lettre']]}
};
// Réglages MIDI de la piste choisie (communs aux pages EDIT · MIDI PISTES et GLO · MIDI)
const mk=()=>isD()?'r'+M.pt:'t'+ti();
const chK=()=>K('CANAL',()=>isD()?rCh(M.pt):tCh(ti()),v=>midiSet(mk(),'mc',v),1,16);
const outK=()=>K('SORTIE',()=>isD()?rOut(M.pt):(trk().mo||0),v=>midiSet(mk(),'mo',v),0,2,1,v=>['INTERNE','FM-1','LES 2'][v]);
const clkK=()=>K('HORLOGE',()=>Math.max(0,['int','ext','auto'].indexOf(S.mcm||'int')),v=>{S.mcm=['int','ext','auto'][v];},0,2,1,v=>['INTERNE','EXTERNE','AUTO'][v]);
const noteN=v=>v+' · '+NOTES[v%12].toUpperCase()+(Math.floor(v/12)-1);
PAGES.emidi={t:'MIDI PISTES',
  knobs:()=>[chK(),outK(),isD()?K('NOTE',()=>rNote(M.pt),v=>midiSet(mk(),'mn',v),0,127,1,noteN):NA,clkK()],
  low:(j,dn)=>{if(!dn)return;
    if(j===0){midiSet('d','mo',1);S.tracks.forEach((t,i)=>midiSet('t'+i,'mo',1));say('Toutes les pistes jouent sur le FM-1');}
    else if(j===1){midiSet('d','mo',0);S.tracks.forEach((t,i)=>midiSet('t'+i,'mo',0));say('Toutes les pistes jouent dans l’app');}
    else if(j===2){midiSet('d','mc',10);S.tracks.forEach((t,i)=>midiSet('t'+i,'mc',i+1));say('Canaux par défaut : mélodie 1 à 6, batterie 10');}
    else if(j===3){S.mclk=S.mclk?0:1;save();say(S.mclk?'Horloge envoyée au FM-1':'Horloge non envoyée');}
    else if(j===4)midiConnect();else if(j===5)midiTest(isD()?rCh(M.pt):tCh(ti()));else if(j===6){midiPanic();say('Notes du FM-1 coupées');}else if(j===7)learnStart();
    midiUI();},
  lab:()=>({low:['TOUT FM-1','TOUT INTERNE','CANAUX DÉFAUT','ENVOI HORL.','CONNECTER','TESTER','STOP NOTES','CALER CLAV.',...blank(8)],on:j=>j===3?!!S.mclk:j===4?!!MIDI.out:j<8}),
  sel:d=>setPt(M.pt+d),click:()=>midiTest(isD()?rCh(M.pt):tCh(ti())),
  help:[['SELECT','choisir la piste (les 8 sons de batterie et les 6 pistes mélodiques) · clic = tester son canal'],['Knobs','canal MIDI (1-16) · sortie : interne / FM‑1 / les deux · note MIDI du son (batterie) · horloge : interne, externe ou auto'],
    ['Touches du bas','1 tout sur le FM‑1 · 2 tout dans l’app · 3 canaux par défaut · 4 envoyer l’horloge · 5 connecter · 6 tester · 7 stop notes · 8 caler le clavier du FM‑1']]};
// ---------- Éditeur de sample (EDIT, page 3) : chaque opération remplace le son (gardé dans ce navigateur), ANNULER revient en arrière ----------
const SE={undo:[],st:0,en:1,g:1.5,loop:0,src:null,t0:0,off:0,len:0};
const seId=()=>M.eid&&bufs[M.eid]?M.eid:(isD()?rowO().smp:trk().smp);
const seBuf=()=>{const id=seId();return id&&id!=='piano'?bufs[id]:null;};
function seCopy(b,a,z){const n=Math.max(1,z-a),o=ctx.createBuffer(b.numberOfChannels,n,b.sampleRate);for(let c=0;c<b.numberOfChannels;c++)o.getChannelData(c).set(b.getChannelData(c).subarray(a,a+n));return o;}
const seRng=b=>{const a=Math.floor(SE.st*b.length);return[a,Math.max(a+1,Math.floor(SE.en*b.length))];};
function seEach(b,fn){const o=seCopy(b,0,b.length),[a,z]=seRng(b);for(let c=0;c<o.numberOfChannels;c++)fn(o.getChannelData(c),a,z);return o;}
function wavOf(b){const ch=b.numberOfChannels,n=b.length,sr=b.sampleRate,buf=new ArrayBuffer(44+n*ch*2),v=new DataView(buf),w=(o,s)=>[...s].forEach((c,i)=>v.setUint8(o+i,c.charCodeAt(0)));
  w(0,'RIFF');v.setUint32(4,36+n*ch*2,true);w(8,'WAVEfmt ');v.setUint32(16,16,true);v.setUint16(20,1,true);v.setUint16(22,ch,true);v.setUint32(24,sr,true);v.setUint32(28,sr*ch*2,true);v.setUint16(32,ch*2,true);v.setUint16(34,16,true);w(36,'data');v.setUint32(40,n*ch*2,true);
  const d=[...Array(ch)].map((_,c)=>b.getChannelData(c));let o=44;for(let i=0;i<n;i++)for(let c=0;c<ch;c++){const x=Math.max(-1,Math.min(1,d[c][i]));v.setInt16(o,x<0?x*32768:x*32767,true);o+=2;}return buf;}
function seStore(id,b){idb('readwrite',s=>s.put(wavOf(b),id)).catch(()=>{});}
function seStop(){if(SE.src){try{SE.src.stop();}catch(e){}SE.src=null;}}
function sePlay(){const b=seBuf();if(!b||!ctx)return say('Pas de sample : choisis-en un dans SON (page 1)');ctx.resume();seStop();const s=ctx.createBufferSource(),g0=ctx.createGain();s.buffer=b;
  const st=SE.st*b.duration,len=Math.max(.01,(SE.en-SE.st)*b.duration);s.connect(g0);g0.connect(master);
  if(SE.loop){s.loop=true;s.loopStart=st;s.loopEnd=st+len;s.start(0,st);}else s.start(0,st,len);SE.src=s;SE.t0=ctx.currentTime;SE.off=st;SE.len=len;s.onended=()=>{if(SE.src===s)SE.src=null;};}
function seOp(name,fn,keepSel){const id=seId(),b=seBuf();if(!b)return say('Pas de sample : choisis-en un dans SON (page 1)');seStop();
  SE.undo.push({id,b,st:SE.st,en:SE.en});if(SE.undo.length>12)SE.undo.shift();const nb=fn(b);bufs[id]=nb;delete rbufs[id];seStore(id,nb);if(!keepSel){SE.st=0;SE.en=1;}save();say(name);}
const SEOPS={
  keep:()=>seOp('✂ Gardé : la sélection devient le son',b=>{const[a,z]=seRng(b);return seCopy(b,a,z);}),
  cut:()=>seOp('✕ Sélection supprimée',b=>{const[a,z]=seRng(b),n=b.length-(z-a);if(n<64)return b;const o=ctx.createBuffer(b.numberOfChannels,n,b.sampleRate);for(let c=0;c<b.numberOfChannels;c++){const d=b.getChannelData(c),x=o.getChannelData(c);x.set(d.subarray(0,a));x.set(d.subarray(z),a);}return o;}),
  rev:()=>seOp('◀◀ Sélection à l’envers',b=>seEach(b,(d,a,z)=>d.subarray(a,z).reverse()),1),
  norm:()=>seOp('Normalisé : le plus fort possible sans saturer',b=>{let p=0;const[a,z]=seRng(b);for(let c=0;c<b.numberOfChannels;c++){const d=b.getChannelData(c);for(let i=a;i<z;i++)p=Math.max(p,Math.abs(d[i]));}const k=p?.97/p:1;return seEach(b,(d,a,z)=>{for(let i=a;i<z;i++)d[i]*=k;});},1),
  fin:()=>seOp('Fondu d’entrée sur la sélection',b=>seEach(b,(d,a,z)=>{for(let i=a;i<z;i++)d[i]*=(i-a)/(z-a);}),1),
  fout:()=>seOp('Fondu de sortie sur la sélection',b=>seEach(b,(d,a,z)=>{for(let i=a;i<z;i++)d[i]*=1-(i-a)/(z-a);}),1),
  sil:()=>seOp('Silence sur la sélection',b=>seEach(b,(d,a,z)=>d.fill(0,a,z)),1),
  gain:()=>seOp('Volume ×'+SE.g.toFixed(2).replace('.',',')+' sur la sélection',b=>seEach(b,(d,a,z)=>{for(let i=a;i<z;i++)d[i]=Math.max(-1,Math.min(1,d[i]*SE.g));}),1),
  undo:()=>{const u=SE.undo.pop();if(!u)return say('Rien à annuler');seStop();bufs[u.id]=u.b;delete rbufs[u.id];seStore(u.id,u.b);SE.st=u.st;SE.en=u.en;say('↶ Annulé');},
  copy:()=>{const id=seId(),b=seBuf();if(!b)return say('Pas de sample');const l=S.lib.find(x=>x.id===id)||{name:'son',f:'Samples'},[a,z]=seRng(b),nb=seCopy(b,a,z),nid='s'+Date.now().toString(36)+rnd(9999);
    bufs[nid]=nb;S.lib.push({id:nid,name:(l.name+' éd.').slice(0,40),f:l.f});seStore(nid,nb);renderLib();save();say('＋ Nouveau son « '+l.name+' éd. » dans '+l.f+' (l’original est gardé)');}};
PAGES.sedit={t:'ÉDITER',
  knobs:()=>[K('DÉBUT',()=>SE.st,v=>{SE.st=Math.min(v,SE.en-.01);},0,1,.002,v=>pct(v)),K('FIN',()=>SE.en,v=>{SE.en=Math.max(v,SE.st+.01);},0,1,.002,v=>pct(v)),
    K('GAIN',()=>SE.g,v=>{SE.g=v;},.1,4,.05,v=>'×'+v.toFixed(2).replace('.',',')),K('ÉCOUTE',()=>SE.loop,v=>{SE.loop=v;},0,1,1,v=>v?'EN BOUCLE':'UNE FOIS')],
  low:(j,dn)=>{if(!dn)return;[sePlay,seStop,SEOPS.keep,SEOPS.cut,SEOPS.rev,SEOPS.norm,SEOPS.fin,SEOPS.fout,SEOPS.sil,SEOPS.gain,()=>{SE.st=0;SE.en=1;},SEOPS.undo,SEOPS.copy][j]?.();},
  lab:()=>({low:['▶ ÉCOUTER','■ STOP','✂ GARDER','✕ SUPPR.','◀◀ REVERSE','NORMALISER','FONDU ↗','FONDU ↘','SILENCE','GAIN ×','TOUT','↶ ANNULER','＋ COPIE',...blank(3)],on:j=>j<13,col:()=>catC(catOfId(seId()||''))}),
  sel:d=>{const w=SE.en-SE.st,st=clamp(SE.st+d*.01,0,1-w);SE.st=st;SE.en=st+w;},click:sePlay,
  help:[['Éditer le son de la piste','KNOB1 début · KNOB2 fin de la sélection · SELECT déplace la sélection · clic = écouter'],['Touches du bas','1 écouter · 2 stop · 3 garder la sélection (couper autour) · 4 supprimer la sélection · 5 reverse · 6 normaliser · 7 fondu d’entrée · 8 fondu de sortie · 9 silence · 10 gain (KNOB3) · 11 tout sélectionner · 12 annuler · 13 copie en nouveau son'],['Sauvegarde','chaque modification est gardée dans ce navigateur · ✂ dans la bibliothèque ouvre ici n’importe quel son']]};
// ---------- ARP, page 1 : les styles, chacun avec sa scène animée ----------
PAGES.gsty={t:'STYLES',
  knobs:()=>[K('STYLE',()=>M.sty,v=>{M.sty=v;},0,STY.length-1,1,v=>ab(STY[v],10)),bpmK(),K('AUTO RYT.',()=>Math.max(0,AUS.indexOf(S.auto||0)),v=>{S.auto=AUS[v];},0,AUS.length-1,1,v=>AUS[v]?AUS[v]+' MES':'OFF'),swK()],
  low:(j,dn)=>{if(!dn||!STY[j])return;M.sty=j;toast(STY[j].toUpperCase()+' · ARP MAINTENU = VALIDER');},
  lab:()=>({low:STY.slice(0,16).map(s=>ab(s,10)),on:j=>STY[j]===S.style,q:j=>j===M.sty&&STY[j]!==S.style}),
  sel:d=>{M.sty=(M.sty+d+STY.length)%STY.length;},
  click:longArp,
  help:[['SELECT','parcourir les styles (aperçu animé) · clic ou ARP maintenu 2 s = valider : tempo, gamme, nouveau rythme et nouvelles mélodies'],['Touches du bas','les 16 premiers styles en direct · le 17e (tribe) avec SELECT'],['Knobs','style · BPM · auto-random du rythme · swing']]};
// ---------- ALGORITHM : un rôle utile partout, choisi dans GLO (page TEMPO, touches 3-8) ----------
const ALGO=[['pages','PAGES','naviguer dans toutes les pages'],['bpm','BPM','tempo'],['dj','FILTRE DJ','filtre général gauche / droite'],['vol','VOLUME','volume de la piste choisie'],['ryt','RYTHMES','rythme de la banque'],['swing','SWING','swing']];
const ORDER=()=>Object.values(GRP).flat();
function algoTurn(dl){const m=S.algo||'pages';
  if(m==='pages'){const L=ORDER(),k=((L.indexOf(M.page)+dl)%L.length+L.length)%L.length;goPage(L[k]);const G=GRP[M.grp];toast(M.grp+' '+(G.indexOf(M.page)+1)+'/'+G.length+' · '+PAGES[M.page].t);return;}
  if(m==='bpm'){kTurn(bpmK(),dl);toast('BPM '+S.bpm);}else if(m==='dj'){const p=djK();kTurn(p,dl);toast('FILTRE DJ '+p.fmt(p.get()));}
  else if(m==='vol'){const p=volK();kTurn(p,dl);toast('VOLUME '+nameOf(M.pt)+' '+p.fmt(p.get()));}else if(m==='swing'){const p=swK();kTurn(p,dl);toast('SWING '+p.fmt(p.get()));}else turn('ALGO_RYT',dl);}
function algoAng(){if(M.page==='hfx')return .1+(M.fxpg||0)*.27;if(LIVEP.includes(M.page))return M.kp?.85:.15;const m=S.algo||'pages';if(m==='pages'){const L=ORDER();return L.indexOf(M.page)/(L.length-1);}if(m==='bpm')return(S.bpm-60)/160;if(m==='dj'){const p=djK();return(p.get()+1)/2;}if(m==='vol')return volK().get();if(m==='swing')return(S.swing||0)/.6;return S.rhythms.length>1?S.cur/(S.rhythms.length-1):0;}
PAGES.gvis={t:'VISU',
  knobs:()=>[bpmK(),djK(),swK(),K('AUTO RYT.',()=>Math.max(0,AUS.indexOf(S.auto||0)),v=>{S.auto=AUS[v];},0,AUS.length-1,1,v=>AUS[v]?AUS[v]+' MES':'OFF')],
  low:(j,dn)=>{if(!dn)return;if(j===0)rndRhythm();else if(j===1){S.tracks.forEach((t,i)=>{randomMel(i);delete LP['t'+i];});say('Nouvelles mélodies sur les accords');}else if(j===2)longArp();else if(j===3){$('#vary').onclick();say('Rythme varié');}else if(j===4){S.tracks.forEach((t,i)=>varyMel(i));say('Mélodies variées');}else if(j===5)toggle();},
  lab:()=>({low:['🎲 RYTHME','🎲 MÉLODIES','🎲 TOUT','VARIER RYT.','VARIER MÉL.',playing?'■ STOP':'▶ PLAY',...blank(10)],on:j=>j<6}),
  sel:d=>{const k=(STY.indexOf(S.style)+d+STY.length)%STY.length;M.sty=k;toast(STY[k].toUpperCase()+' · ARP MAINTENU = VALIDER');},click:longArp,
  help:[['Visu','l’animation du style joué, sur le tempo'],['Touches du bas','1 🎲 rythme · 2 🎲 mélodies · 3 🎲 tout · 4 varier le rythme · 5 varier les mélodies · 6 play / stop'],['ARP maintenu 2 s','valider le style choisi et tout regénérer']]};
// ---------- FX, page 1 : le récap de tous les effets, chacun coupable d'une touche ; FX maintenu = remettre à zéro ----------
const anyT=(f)=>S.tracks.some(f)||S.rows.some(f);
const FXL=[
  ['COUPE-HAUT',()=>1-S.mf,()=>{S.mf=1;}],['COUPE-BAS',()=>S.hp||0,()=>{S.hp=0;}],['RÉSONANCE',()=>S.mq||0,()=>{S.mq=0;}],
  ['ÉCHO',()=>S.xd||0,()=>{S.xd=0;}],['REVERB',()=>S.xr||0,()=>{S.xr=0;}],['DISTO',()=>S.xs||0,()=>{S.xs=0;}],
  ['DISTO BATT.',()=>S.dfx.dist,()=>{S.dfx.dist=0;}],['DELAY BATT.',()=>S.dfx.dly,()=>{S.dfx.dly=0;}],['REVERB BATT.',()=>S.dfx.rev>.061?S.dfx.rev:0,()=>{S.dfx.rev=.06;}],
  ['LOOP · ROLL',()=>anyT(o=>(o.lp||0)>0||(o.rl||1)>1)?1:0,()=>{[...S.tracks,...S.rows].forEach(o=>{o.lp=0;o.rl=1;});LP={};}],
  ['DISTO PISTES',()=>Math.max(...S.tracks.map(t=>t.dist||0)),()=>{S.tracks.forEach(t=>t.dist=0);}],
  ['ÉCHO PISTES',()=>Math.max(...S.tracks.map(t=>t.dly||0)),()=>{S.tracks.forEach(t=>t.dly=0);}],
  ['REVERB PISTES',()=>Math.max(...S.tracks.map(t=>t.rev||0)),()=>{S.tracks.forEach(t=>t.rev=0);}],
  ['LFO PISTES',()=>Math.max(...S.tracks.map(t=>t.lfd||0)),()=>{S.tracks.forEach(t=>t.lfd=0);}],
  ['MOUVEMENTS',()=>Object.keys(S.am.d).length?1:0,()=>{$('#amClr').onclick();}]];
function fxCut(j){const e=FXL[j];if(!e)return;e[2]();['mf','hp','mq','xd','xr','xs'].forEach(k=>{const el=$('#'+k);if(el)el.value=S[k];});applyParams();save();say(e[0]+' coupé');}
function fxCutAll(){FXL.forEach(e=>e[2]());lvUp();['lp','rl','rv'].forEach(k=>lvSet(k,0));if(latch)$('#lat').onclick();S.lt='';['mf','hp','mq','xd','xr','xs'].forEach(k=>{const el=$('#'+k);if(el)el.value=S[k];});applyParams();save();say('Tous les effets coupés');}
// FX maintenu 2 s : l'effet de la page affichée revient à zéro
function longFx(){const p=M.page;
  if(p==='fxa'){S.mf=1;S.hp=0;S.mq=0;}else if(p==='fxb'){S.xd=0;S.xr=0;S.xs=0;}else if(p==='fxd'){S.dfx={vol:.9,dist:0,dly:0,rev:.06};}
  else if(p==='fxp'){lvUp();['lp','rl','rv'].forEach(k=>lvSet(k,0));if(latch)$('#lat').onclick();S.lt='';LP={};}else{fxCutAll();M.flash=performance.now();return;}
  ['mf','hp','mq','xd','xr','xs'].forEach(k=>{const el=$('#'+k);if(el)el.value=S[k];});document.querySelectorAll('[data-dp]').forEach(el=>{el.value=S.dfx[el.dataset.dp];});applyParams();save();M.flash=performance.now();say(PAGES[p].t+' remis à zéro');}
PAGES.fxr={t:'FX RÉCAP',
  knobs:()=>[djK(),gk('ÉCHO','xd'),gk('REVERB','xr'),gk('DISTO','xs')],
  low:(j,dn)=>{if(!dn)return;if(j<FXL.length)fxCut(j);else if(j===15)fxCutAll();},
  lab:()=>({low:[...FXL.map(e=>e[0]),'','TOUT COUPER'],on:j=>j<FXL.length?FXL[j][1]()>.005:j===15,q:j=>j===15}),
  sel:d=>{const L=GRP.FX,k=(L.indexOf(M.page)+d+L.length)%L.length;goPage(L[k],1);},click:()=>{},
  help:[['Récap','tous les effets en cours : allumé = actif, avec sa valeur'],['Touches du bas','chaque touche coupe son effet · 16 = tout couper'],['FX maintenu 2 s','remet à zéro l’effet de la page affichée (sur le récap : tout)'],['Knobs','filtre DJ · écho · reverb · disto']]};
// ---------- LIVE, page 4 : FX LIVE · 8 effets (2 pages de 4 knobs, ALGORITHM), sur une cible au choix (SELECT) ----------
const FXT=[['RYTHME','r'],['MÉLODIE','m'],['BASSE',0],['BASSE 2',2],['SYNTHÉ',1],['SYNTHÉ 2',3],['NAPPE',4],['FX',5]];
/* [clé, nom, valeur coupée, choix (knob à crans)] — pages A B : le son · C D : le jeu */
const FXE=[[['cut','FILTRE',1],['res','RÉSO',0],['dist','DISTO',0],['dly','ÉCHO',0]],[['rev','REVERB',0],['lfd','WOBBLE',0],['lfr','VITESSE',null],['vol','VOLUME',null]],
  [['cho','CHORUS',0],['rl','ROLL',0,[[0,'OFF'],[2,'×2'],[3,'×3'],[4,'×4']]],['sk','PAS SAUTÉS',0],['rv','REVERSE',0,[[0,'OFF'],[1,'ON']]]],
  [['lp','LOOP',0,[[0,'OFF'],[1,'1 PAS'],[2,'2 PAS'],[4,'4 PAS'],[8,'8 PAS']]],['cr','BITCRUSH',0],['gt','GATE',0],['hf','DEMI-TEMPO',0,[[0,'OFF'],[1,'ON']]]]];
const FXPN=['A','B','C','D'],FXPT=['FILTRE · RÉSO · DISTO · ÉCHO','REVERB · WOBBLE · VITESSE · VOLUME','CHORUS · ROLL · PAS SAUTÉS · REVERSE','LOOP · BITCRUSH · GATE · DEMI-TEMPO'];
const ROWK=['lp','rl','rv','sk','hf'];
const fxtC=j=>{const k=FXT[j][1];return k==='r'?COL.c:k==='m'?COL.a:trkC(8+k);};
const fxObjs=e=>{const k=FXT[M.fxt||0][1];return k==='r'?(ROWK.includes(e)?S.rows:[S.dfx]):k==='m'?S.tracks:[S.tracks[k]];};
const fxDef=(o,k)=>o[k]==null?(k==='cut'?1:k==='lfr'?.3:k==='vol'?.8:0):o[k];
const fxGet=k=>{const L=fxObjs(k);return L.reduce((a,o)=>a+fxDef(o,k),0)/L.length;};
function fxSetV(k,v){fxObjs(k).forEach(o=>o[k]=v);if(ROWK.includes(k)&&typeof LP!=='undefined')for(const x in LP)delete LP[x];applyParams();}
const fxIx=e=>{const v=fxGet(e[0]);let b=0;e[3].forEach((o,i)=>{if(Math.abs(o[0]-v)<Math.abs(e[3][b][0]-v))b=i;});return b;};
const fxK=e=>e[3]?K(e[1],()=>fxIx(e),v=>fxSetV(e[0],e[3][Math.round(v)][0]),0,e[3].length-1,1,v=>e[3][Math.round(v)][1]):K(e[1],()=>fxGet(e[0]),v=>fxSetV(e[0],v),0,1,.01,pct);
M.byp={};
function fxByp(e){const key=(M.fxt||0)+':'+e[0];if(e[2]==null)return say(e[1]+' : pas de coupure, règle-le au knob');
  if(M.byp[key]!=null){fxSetV(e[0],M.byp[key]);delete M.byp[key];say(e[1]+' rétabli sur '+FXT[M.fxt||0][0]);}else{M.byp[key]=fxGet(e[0]);fxSetV(e[0],e[2]);say(e[1]+' coupé sur '+FXT[M.fxt||0][0]+' (rappuie pour le rétablir)');}save();}
PAGES.hfx={t:'FX LIVE',
  knobs:()=>FXE[M.fxpg||0].map(fxK),
  low:(j,dn)=>{if(!dn)return;if(j<8){M.fxt=j;say('Effets sur : '+FXT[j][0]);}else{const h=(M.fxpg||0)>>1,e=[...FXE[2*h],...FXE[2*h+1]][j-8];fxByp(e);}},
  lab:()=>{const h=(M.fxpg||0)>>1,all=[...FXE[2*h],...FXE[2*h+1]];return{low:[...FXT.map(t=>t[0]),...all.map(e=>(M.byp[(M.fxt||0)+':'+e[0]]!=null?'↺ ':'')+e[1])],
    on:j=>j<8?j===(M.fxt||0):all[j-8][2]!=null&&Math.abs(fxGet(all[j-8][0])-all[j-8][2])>.01,col:j=>j<8?fxtC(j):fxtC(M.fxt||0),q:j=>j>=8&&M.byp[(M.fxt||0)+':'+all[j-8][0]]!=null};},
  sel:d=>{M.fxt=((M.fxt||0)+d+8)%8;toast('EFFETS SUR : '+FXT[M.fxt][0]);},click:()=>{},
  help:[['SELECT ou touches 1-8','la cible : rythme, toute la mélodie, ou une piste (basse, basse 2, synthé, synthé 2, nappe, fx)'],['ALGORITHM','4 pages : A filtre · réso · disto · écho — B reverb · wobble · vitesse · volume — C chorus · roll · pas sautés · reverse — D loop · bitcrush · gate · demi-tempo'],['Touches 9-16','couper / rétablir les 8 effets des pages A+B (ou C+D quand tu y es)']]};
window.recFlash=(i,st,n)=>{M.rf={p:n<0?i:8+i,st,n,t:performance.now()};panelDirty=1;};
/* PRESETS sur les pages LIVE : les sons du dossier de la piste (CLAP → le dossier Clap…), son interne compris */
function presetSnd(d){const dr=isD(),cat=dr?ROWCAT[M.pt]:TYPCAT[trk().type],o=dr?rowO():trk();
  const L=['',...(dr?[]:['piano']),...S.lib.filter(l=>l.f===cat).map(l=>l.id)],k=Math.max(0,L.indexOf(o.smp||'')),id=L[((k+d)%L.length+L.length)%L.length];
  if(dr){o.smp=id;drum(M.pt,now()+.01,1);}else withSel(ti(),()=>setSmp(id));save();
  const l=S.lib.find(x=>x.id===id);say(L.length<(dr?2:3)?nameOf(M.pt)+' : son interne · ajoute des sons dans le dossier « '+cat+' » (Bibliothèque) pour les faire défiler avec PRESETS':nameOf(M.pt)+' '+(L.indexOf(id)+1)+'/'+L.length+' : '+(id==='piano'?'Piano':l?l.name:dr?'son interne':'synthé interne'));panelDirty=1;}
const koK=()=>K('OCT CLAV.',()=>M.oct,v=>{M.oct=v;},-2,2,1,v=>(v>0?'+':'')+v);
const page=()=>PAGES[M.page];
function goPage(p,keep){const P=PAGES[p];
  if(P.sc==='r'&&!isD())M.pt=M.lastRow;if(P.sc==='t'&&isD()){M.pt=8+M.lastTrk;S.sel=M.lastTrk;}
  if(!keep)lvUp();M.held.clear();M.page=p;M.grp=grpOf(p);M.sub[M.grp]=GRP[M.grp].indexOf(p);
  if(p==='son'||p==='sonr')LIBP.enter();if(p==='gsty')M.sty=Math.max(0,STY.indexOf(S.style));if(p!=='sedit')seStop();if(p==='nom'&&M.nr<0)M.nr=S.cur;if(p!=='nom'&&M.page!=='bank')M.nr=-1;if(p==='bank'&&!M.bi)M.bi=0;if(p==='ryt'||p==='mel')M.cur=Math.max(0,curStep);
  panelDirty=1;}
// Un bouton : sa dernière page, puis la suivante à chaque nouvel appui
function pageBtn(b){const L=GRP[b];if(M.grp===b)goPage(L[(L.indexOf(M.page)+1)%L.length]);else goPage(L[M.sub[b]||0]);}
['flt','lfo','osc'].forEach(p=>{PAGES[p].low=PAGES.env.low;PAGES[p].lab=PAGES.env.lab;});

/* ---------- Touches ---------- */
// ---------- Mode JEU (pages LIVE) : ALGORITHM à droite = les 27 touches jouent l'instrument choisi, à gauche = retour aux coupures ----------
const LIVEP=['home','hryt','hmel'],kpOn=()=>M.kp&&LIVEP.includes(M.page);
const kpDeg=r=>r+M.oct*SL();                       /* r = position de la touche de gauche à droite (0..26) : la gamme qui monte */
const KP_HELD=new Set();
function recDrum(k,v){if(!(rec&&playing&&curStep>=0))return;const st=(ctx.currentTime-stT)/(60/S.bpm/4)>.5?(curStep+1)%16:curStep;cur().g[k][st]=v;cur().tmp=false;save();recFlash(k,st,-1);}
function kpKey(r,dn,acc){if(isD()){if(!dn)return;const k=r;drum(k,now()+.005,acc?2:1);recDrum(k,acc?2:1);return;}
  if(dn){KP_HELD.add(r);M.deg=Math.max(0,kpDeg(r));liveOn(kpDeg(r));}else{KP_HELD.delete(r);if(!KP_HELD.size)liveOff();else{const last=[...KP_HELD].pop();liveOn(kpDeg(last));}}}
function kpSet(on){M.kp=!!on;KP_HELD.clear();liveOff();panelDirty=1;say(M.kp?'🎹 Mode JEU : tout le clavier joue '+nameOf(M.pt)+(isD()?' (touches 9-16 = accent)':' dans la gamme')+' · ALGORITHM à gauche pour revenir':'Mode COUPURES : les touches du bas coupent et relancent les pistes');}
function upKey(k,dn){if(kpOn()&&!isD()){if(dn)M.upDown.add(k);else M.upDown.delete(k);return kpKey(BLACK[k],dn);}
  if(dn)M.upDown.add(k);else M.upDown.delete(k);
  if(isD()){if(k>7||!dn)return;if(M.pt!==k)M.pt=k;drum(k,now()+.005,1);
    recDrum(k,1);return;}
  const d=k+M.oct*SL();
  if(dn){M.deg=Math.max(0,d);liveOn(d);
    if(M.page==='mel'&&M.held.size){const tr=trk();M.held.forEach((_,j)=>{const s=tr.steps[j];tr.steps[j]={n:Math.max(0,d),a:s.a||0,s:s.s||0,o:0,l:s.n>=0?s.l||1:1};M.held.set(j,true);});save();}}
  else if(!M.upDown.size)liveOff();}
function lowKey(j,dn){if(dn)M.lowDown.add(j);else M.lowDown.delete(j);if(kpOn()){if(isD())return kpKey(j%8,dn,j>=8);return kpKey(WHITE[j],dn);}page().low(j,dn);}
function longHome(){if(isD())rndRow(M.pt);else rndTrk(ti());M.flash=performance.now();}
// ARP maintenu : valide le style choisi (page STYLES) puis regénère le rythme et toutes les mélodies
function longArp(){const st=M.grp==='ARP'&&M.page==='gsty'?STY[M.sty]:S.style;setStyle(st);styleSc();randomRhythm();S.tracks.forEach((t,i)=>{randomMel(i);delete LP['t'+i];});M.sty=STY.indexOf(st);save();M.flash=performance.now();
  say('🎲 Style '+st+' : '+S.bpm+' BPM · gamme '+S.scale+' · nouveau rythme et nouvelles mélodies'+(playing?' (le rythme change à la mesure)':' — PLAY pour écouter'));}
const LONG={FX:{fn:longFx,l:()=>'↺ MAINTIENS : '+(M.grp==='FX'&&M.page!=='fxr'?PAGES[M.page].t+' À ZÉRO':'COUPER TOUS LES EFFETS')},HOME:{fn:longHome,l:()=>'🎲 MAINTIENS : '+(isD()?'NOUVEAU MOTIF ':'NOUVELLE MÉLODIE ')+nameOf(M.pt)},ARP:{fn:longArp,l:()=>'🎲 MAINTIENS : '+(M.page==='gsty'?STY[M.sty]:S.style).toUpperCase()+' · TOUT REGÉNÉRER'}};
function btn(n,dn){
  if(LONG[n]){if(dn){clearTimeout(M.hhT);M.hh={b:n,t0:performance.now(),done:false};M.hhT=setTimeout(()=>{if(M.hh){M.hh.done=true;LONG[n].fn();}},2000);}
    else{clearTimeout(M.hhT);const h=M.hh;M.hh=null;if(h&&!h.done)pageBtn(n);}return;}
  if(!dn)return;
  if(GRP[n])return pageBtn(n);
  if(n==='PLAY'){toggle();return;}
  if(n==='REC'){if(rec||arec){if(rec)$('#rec').onclick();if(arec)amRec();say('REC coupé : tes mouvements rejouent en boucle (FX : REJOUE / EFFACER)');}
    else{$('#rec').onclick();amRec();say('REC : les notes jouées s’écrivent dans '+nameOf(M.pt)+' et les knobs tournés sont enregistrés en boucle');}return;}
  if((n==='OCT-'||n==='OCT+')&&(LIVEP.includes(M.page)||M.page==='hfx')){muteGrp(n==='OCT-'?'r':'t');panelDirty=1;return;}
  if(n==='OCT-'||n==='OCT+'){M.oct=clamp(M.oct+(n==='OCT+'?1:-1),-2,2);toast('OCTAVE CLAVIER '+(M.oct>0?'+':'')+M.oct);}}
function turn(n,dl){if(!dl||!S)return;
  if(n==='MASTER'){S.mv=clamp((S.mv==null?.8:S.mv)+dl*.02,0,1);applyParams();save();toast('MASTER '+pct(S.mv));return;}
  if(n==='SELECT')return page().sel(dl);
  if(n==='PRESETS'){if(LIVEP.includes(M.page))return presetSnd(dl>0?1:-1);return setPt(M.pt+dl);}
  if(n==='ALGORITHM'){if(M.page==='hfx'){const o=M.fxpg||0;M.fxpg=Math.max(0,Math.min(3,o+(dl>0?1:-1)));M.fxd=M.fxpg>=o?1:-1;M.fxa=performance.now();panelDirty=1;return toast('FX LIVE · PAGE '+FXPN[M.fxpg]+' : '+FXPT[M.fxpg]);}if(LIVEP.includes(M.page))return kpSet(dl>0);return algoTurn(dl);}
  if(n==='ALGO_RYT'){const b=queued?S.rhythms.indexOf(queued):S.cur,k=(b+dl+S.rhythms.length*9)%S.rhythms.length;go(S.rhythms[k]);toast('RYTHME '+(k+1)+' · '+ab(S.rhythms[k].name,18));return;}
  const m=/^KNOB(\d)$/.exec(n);if(m){const i=m[1]-1,p=page().knobs()[i];kTurn(p,dl);amK(i,p);}}
/* Mouvements des knobs : REC les enregistre (pages LIVE, FX, ENV, MIX, SON), ils rejouent en boucle avec ceux des effets */
const AMP=['hfx','home','hryt','hmel','fxa','fxb','fxd','fxp','env','flt','lfo','osc','mix','lrr','son','sonr'];
const amKey=i=>AMP.includes(M.page)?'k:'+(M.page==='hfx'?'hfx/'+(M.fxpg||0)+'/'+(M.fxt||0):M.page)+':'+M.pt+':'+i:'';
function amK(i,p){if(!arec||!p||p.g||!p.l)return;const k=amKey(i);if(!k||aT[k])return;aT[k]=1;if(!S.am.d[k])S.am.d[k]=Array((S.am.n||4)*16).fill(null);amUI();}
function amAt(k,fn){const[,pq,pt,i]=k.split(':'),[pg,fa,fb]=pq.split('/'),sp=M.pt,ss=S.sel,sa=M.fxpg,sb=M.fxt;M.pt=+pt;if(M.pt>=8)S.sel=M.pt-8;if(fa!=null){M.fxpg=+fa;M.fxt=+fb;}
  try{const p=PAGES[pg]&&PAGES[pg].knobs()[+i];return p&&fn(p);}finally{M.pt=sp;S.sel=ss;M.fxpg=sa;M.fxt=sb;}}
window.amGetK=k=>amAt(k,p=>p.get());
window.amSetK=(k,v)=>amAt(k,p=>{if(Math.abs(p.get()-v)>1e-9)p.set(v);});
// Commandes venues du vrai FM-1 (USB MIDI), après l'apprentissage des knobs
window.fm1Ext={
  note:(n,on)=>{let r=n-MIDI.base;while(r<0)r+=12;while(r>26)r-=12;const w=WHITE.indexOf(r);if(w>=0)lowKey(w,on);else upKey(BLACK.indexOf(r),on);flashKey(w>=0?'l'+w:'u'+BLACK.indexOf(r),on);},
  ctl:(k,t,v)=>{let dl=0;
    if(t==='r64')dl=v-64;else if(t==='r2')dl=v<64?v:v-128;
    else{const m=/^KNOB(\d)$/.exec(k);if(m){const p=page().knobs()[m[1]-1];kAbs(p,v/127);amK(m[1]-1,p);return;}if(k==='MASTER'){S.mv=v/127;applyParams();save();return;}
      const l=M.last[k];M.last[k]=v;if(l==null)return;dl=v-l;if(Math.abs(dl)>12)return;}
    turn(k,dl);}};

/* ---------- Le FM-1 virtuel (DOM) ---------- */
let cv,g,panelDirty=1,keySig='',knobSig='',DPR=1;
const KN=['MASTER','SELECT','PRESETS','ALGORITHM'],KR=['KNOB1','KNOB2','KNOB3','KNOB4'],B1=['FX','SCL','ENV','LFO','EDIT','GLO'],B2=['HOME','SAVE','ARP','SEQ','PLAY','REC'];
const BSUB={HOME:'LIVE ×4',SEQ:'RYT/MÉL',ARP:'STYLE ×4',FX:'FX ×5',SCL:'GAMME',ENV:'ENV ×4',LFO:'MIX ×2',EDIT:'SON ×4',GLO:'TEMPO/MIDI',SAVE:'BANQUE ×4'};
// Raccourcis du clavier de l'ordinateur (positions physiques, valables en AZERTY comme en QWERTY)
const KC_UP=['KeyQ','KeyW','KeyE','KeyR','KeyT','KeyY','KeyU','KeyI','KeyO','KeyP','BracketLeft'];
const KC_LOW=['KeyA','KeyS','KeyD','KeyF','KeyG','KeyH','KeyJ','KeyK','KeyZ','KeyX','KeyC','KeyV','KeyB','KeyN','KeyM','Comma'];
const KC_BTN={Digit1:'FX',Digit2:'SCL',Digit3:'ENV',Digit4:'LFO',Digit5:'EDIT',Digit6:'GLO',Digit7:'HOME',Digit8:'SAVE',Digit9:'ARP',Digit0:'SEQ',Space:'PLAY',Minus:'REC',Escape:'HOME'};
const AZ={KeyQ:'A',KeyW:'Z',KeyE:'E',KeyR:'R',KeyT:'T',KeyY:'Y',KeyU:'U',KeyI:'I',KeyO:'O',KeyP:'P',BracketLeft:'^',KeyA:'Q',KeyS:'S',KeyD:'D',KeyF:'F',KeyG:'G',KeyH:'H',KeyJ:'J',KeyK:'K',KeyZ:'W',KeyX:'X',KeyC:'C',KeyV:'V',KeyB:'B',KeyN:'N',KeyM:',',Comma:';',Digit1:'1',Digit2:'2',Digit3:'3',Digit4:'4',Digit5:'5',Digit6:'6',Digit7:'7',Digit8:'8',Digit9:'9',Digit0:'0',Minus:')'};
let KL=Object.assign({},AZ);
function fm1Build(){const dev=$('#dev');
  const knob=(n,cls)=>`<div class="kw ${cls||''}"><b>${n}${n==='ALGORITHM'?'<small data-ks></small>':''}</b><div class="kn" data-kn="${n}" title="${n} : glisse vers le haut / le bas ou molette${n==='SELECT'?' · clic = valider':''}"><i></i></div></div>`;
  const bt=n=>{const kc=Object.keys(KC_BTN).find(k=>KC_BTN[k]===n&&k!=='Escape');return`<button class="fb" data-fb="${n}" title="${n}${BSUB[n]?' ('+BSUB[n]+')':''} · touche ${KL[kc]||kc}">${n}<small>${BSUB[n]||''}</small><kbd>${KL[kc]||(n==='PLAY'?'␣':'')}</kbd></button>`;};
  dev.innerHTML=`<div class="dbody">
    <div class="dl"><div class="kg">${KN.map(n=>knob(n,n==='SELECT'?'sel':'')).join('')}</div><div class="oct"><button class="fb" data-fb="OCT-">OCT-</button><button class="fb" data-fb="OCT+">OCT+</button></div></div>
    <div class="dsc"><canvas id="scr" width="240" height="240"></canvas><span class="brand">FM‑1 · GROOVE</span></div>
    <div class="dr"><div class="kr">${KR.map(n=>knob(n,'k4')).join('')}</div><div class="bg">${[...B1,...B2].map(bt).join('')}</div></div>
    <div class="kb"><div class="ku">${BPOS.map((w,k)=>`<button class="ky u" data-ku="${k}" style="left:calc(${(w+1)/16*100}% - 2.6%)"><span></span><kbd>${KL[KC_UP[k]]}</kbd></button>`).join('')}</div>
      <div class="kl">${Array.from({length:16},(_,j)=>`<button class="ky l" data-kl="${j}"><span></span><kbd>${KL[KC_LOW[j]]}</kbd></button>`).join('')}</div></div></div>`;
  cv=$('#scr');g=cv.getContext('2d');sizeCanvas();
  // Knobs : glisser (haut/bas), molette, clic sur SELECT
  dev.querySelectorAll('.kn').forEach(el=>{const n=el.dataset.kn;let y0=0,used=0,mv=false,id=-1;
    const px=()=>{if(!KR.includes(n))return n==='MASTER'?3:14;const p=page().knobs()[KR.indexOf(n)],r=(p.mx-p.mn)/p.st;return r>120?4:r>40?3:14;};
    el.addEventListener('pointerdown',e=>{e.preventDefault();ctx&&ctx.resume();el.setPointerCapture(e.pointerId);id=e.pointerId;y0=e.clientY;used=0;mv=false;el.classList.add('on');if(KR.includes(n))aH=amKey(KR.indexOf(n));});
    el.addEventListener('pointermove',e=>{if(e.pointerId!==id)return;const dy=y0-e.clientY,s=Math.trunc(dy/px())-used;if(Math.abs(dy)>4)mv=true;if(s){used+=s;turn(n,s);}});
    const end=e=>{if(e.pointerId!==id)return;id=-1;aH='';el.classList.remove('on');if(!mv&&n==='SELECT')page().click();};
    el.addEventListener('pointerup',end);el.addEventListener('pointercancel',end);
    el.addEventListener('wheel',e=>{e.preventDefault();turn(n,e.deltaY<0?1:-1);},{passive:false});});
  panelDirty=1;}
let bound=false;
function fm1Bind(){if(bound)return;bound=true;const dev=$('#dev');
  // Boutons et touches : appui / relâché (les touches se jouent aussi en glissant le doigt)
  const press=(el,dn)=>{const d=el.dataset;if(d.fb)btn(d.fb,dn);else if(d.ku!=null)upKey(+d.ku,dn);else if(d.kl!=null)lowKey(+d.kl,dn);el.classList.toggle('dn',dn);};
  const act=new Map();
  dev.addEventListener('pointerdown',e=>{const el=e.target.closest('.fb,.ky');if(!el)return;e.preventDefault();ctx&&ctx.resume();try{el.releasePointerCapture(e.pointerId);}catch(_){}act.set(e.pointerId,el);press(el,true);});
  dev.addEventListener('pointerover',e=>{if(!act.has(e.pointerId)||!e.buttons)return;const el=e.target.closest('.ky');const o=act.get(e.pointerId);if(!el||el===o)return;press(o,false);act.set(e.pointerId,el);press(el,true);});
  const up=e=>{const o=act.get(e.pointerId);if(!o)return;act.delete(e.pointerId);press(o,false);};
  document.addEventListener('pointerup',up);document.addEventListener('pointercancel',up);
  dev.addEventListener('contextmenu',e=>{if(e.target.closest('.fb,.ky,.kn'))e.preventDefault();});
  // Glisser des fichiers audio sur le FM-1 = les ajouter à la bibliothèque
  dev.addEventListener('dragover',e=>{e.preventDefault();dev.classList.add('drag');});
  dev.addEventListener('dragleave',()=>dev.classList.remove('drag'));
  dev.addEventListener('drop',async e=>{e.preventDefault();dev.classList.remove('drag');const fl=await dropFiles(e.dataTransfer);
    if(!fl.length)return;addFiles(fl,false,(M.page==='son'||M.page==='sonr')&&M.lib.lvl?M.lib.f:'');if(M.grp!=='EDIT')goPage('son');});}
function flashKey(id,on){const el=$(`#dev [data-k${id[0]}="${id.slice(1)}"]`);if(el)el.classList.toggle('dn',on);}
function sizeCanvas(){DPR=Math.min(3,(window.devicePixelRatio||1)*(cv.clientWidth/240||1));cv.width=cv.height=Math.round(240*DPR);}

/* ---------- Écran ---------- */
function tx(s,x,y,c,sz,al,wt){g.fillStyle=c||COL.t;g.font=(wt||700)+' '+(sz||10)+'px '+FONT;g.textAlign=al||'left';g.fillText(s,x,y);}
function fit(s,w,sz,wt){s=String(s);g.font=(wt||700)+' '+(sz||10)+'px '+FONT;if(g.measureText(s).width<=w)return s;while(s.length>1&&g.measureText(s+'…').width>w)s=s.slice(0,-1);return s+'…';}
function rr(x,y,w,h,r,f,s,lw){g.beginPath();g.roundRect(x,y,w,h,r);if(f){g.fillStyle=f;g.fill();}if(s){g.strokeStyle=s;g.lineWidth=lw||1;g.stroke();}}
const hit=k=>{const t=HIT[k];if(t==null||!ctx)return 0;const a=ctx.currentTime-t;return a>=0&&a<.14?1-a/.14:0;};
function draw(){if(!g||!S)return;g.setTransform(DPR,0,0,DPR,0,0);g.fillStyle=COL.bg;g.fillRect(0,0,240,240);
  const P=page(),dr=isD();
  // En-tête : page, piste, lecture, tempo, position
  const GL=GRP[M.grp]||[],gi=GL.indexOf(M.page),dw=GL.length>1?GL.length*6+4:0;g.font='800 11px '+FONT;const cw0=Math.max(40,g.measureText(P.t).width+12+dw);rr(3,3,cw0,17,4,COL.a);tx(P.t,3+(cw0-dw)/2,15.5,'#1a1206',11,'center',800);
  GL.forEach((_,k)=>{g.fillStyle=k===gi?'#1a1206':'rgba(26,18,6,.35)';g.beginPath();g.arc(3+cw0-dw+4+k*6,11.5,2.2,0,7);g.fill();});
  const tc=trkC(M.pt),tx0=cw0+5,tw=Math.max(56,(rec?186:194)-tx0);rr(tx0,3,tw,17,4,null,tc,1.2);tx(fit((dr?'● ':'♪ ')+nameOf(M.pt),tw-8,10),tx0+5,15.5,tc,10);
  tx((playing?'▶':'■')+S.bpm,237,15.5,playing?COL.y:COL.m,11,'right',800);
  if(rec){g.fillStyle=COL.r;g.beginPath();g.arc(tx0+tw+5,11.5,3.5,0,7);g.fill();}
  const p=prg();tx(fit(S.style.toUpperCase()+' · '+NOTES[S.root].toUpperCase()+' '+S.scale.toUpperCase()+(p?' · '+chName(p[playing?cix:0]||0):''),160,9,500),4,31,COL.m,9,'left',500);
  for(let b=0;b<4;b++){const on=playing&&curStep>=0&&Math.floor(curStep/4)===b;rr(194+b*11,24,8,8,2,on?COL.c:COL.p2);}
  g.fillStyle=COL.l;g.fillRect(0,35,240,1);
  (DRAW[M.page]||(()=>{}))();
  if(M.rf&&performance.now()-M.rf.t<900){const f=M.rf,a=Math.min(1,2.2*(1-(performance.now()-f.t)/900)),c=trkC(f.p);g.globalAlpha=a;rr(30,150,180,22,6,'#2a0d0d',COL.r,1.5);g.fillStyle=COL.r;g.beginPath();g.arc(44,161,4+2*Math.sin(performance.now()/60),0,7);g.fill();
    tx('REC '+(f.n<0?ROWS[f.p]:nameOf(f.p)+' '+degName(f.n))+' · PAS '+(f.st+1),128,165,c,8.5,'center',900);g.globalAlpha=1;}
  // Pied : les 4 knobs de la page
  g.fillStyle=COL.l;g.fillRect(0,180,240,1);
  P.knobs().forEach((k,i)=>{const cx=30+i*60,cy=199;if(!k.l)return;const n=k.mx>k.mn?(k.get()-k.mn)/(k.mx-k.mn):0;
    g.lineWidth=4;g.lineCap='round';g.strokeStyle=COL.p2;g.beginPath();g.arc(cx,cy,12,.75*Math.PI,2.25*Math.PI);g.stroke();
    g.strokeStyle=i%2?COL.c:COL.a;g.beginPath();g.arc(cx,cy,12,.75*Math.PI,(.75+1.5*n)*Math.PI);g.stroke();
    tx(fit(k.l,58,9),cx,224,COL.m,9,'center',600);tx(fit(k.fmt(k.get()),58,10),cx,236,COL.t,10,'center',800);
    tx(String(i+1),cx,203,COL.l,9,'center',700);
    if(S.am.d[amKey(i)]||k.g&&S.am.d[['mf','hp','xd','xr','mq','xs'].find(x=>k.l&&gkL[x]===k.l)]){g.fillStyle=COL.c;g.beginPath();g.arc(cx+15,cy-12,3,0,7);g.fill();}});
  if(kpOn()){rr(150,24,44,10,3,COL.y);tx('🎹 JEU',172,32,'#111',8,'center',900);}
  // Appui long sur HOME : jauge, puis éclair quand la génération part
  if(M.hh&&!M.hh.done){const p=Math.min(1,(performance.now()-M.hh.t0)/2000);if(p>.12){rr(6,146,228,28,6,'rgba(6,16,14,.95)',COL.y,1.2);rr(10,163,220*p,6,3,COL.y);tx(fit(LONG[M.hh.b].l(),220,9),120,159,COL.y,9,'center',800);}}
  if(M.flash&&performance.now()-M.flash<400){g.globalAlpha=.35*(1-(performance.now()-M.flash)/400);g.fillStyle=COL.y;g.fillRect(0,36,240,144);g.globalAlpha=1;}
  // Message bref
  if(M.toast&&performance.now()<M.toast.u){const s=fit(M.toast.t,224,10);rr(6,150,228,22,5,'rgba(6,16,14,.94)',COL.c,1);tx(s,120,165,COL.c,10,'center',700);}}
const DRAW={
hfx(){const T=performance.now()/1000,beat=60/S.bpm,pg=M.fxpg||0,tc=fxtC(M.fxt||0),sw=Math.min(1,(performance.now()-(M.fxa||0))/250);
  FXT.forEach((t,j)=>{const x=2+j*29.6,on=j===(M.fxt||0),c=fxtC(j);rr(x,38,27,13,3,on?c:COL.p2);tx(fit(t[0].replace('SYNTHÉ','SYN'),25,6.5),x+13.5,47,on?'#111':c,6.5,'center',800);});
  tx('PAGE '+FXPN[pg],210,62,COL.y,8,'right',900);for(let k=0;k<4;k++){g.fillStyle=k===pg?COL.y:COL.p2;g.beginPath();g.arc(217+k*6,59,2.2,0,7);g.fill();}tx(FXT[M.fxt||0][0],6,62,tc,9,'left',900);
  const cells=FXE[pg];g.save();g.translate((1-sw)*40*(M.fxd||1),0);g.globalAlpha=sw;
  cells.forEach((e,i)=>{const x=4+i*59,y=67,w=56,h=88,v=fxGet(e[0]);rr(x,y,w,h,5,COL.p);g.save();g.beginPath();g.rect(x,y,w,h);g.clip();
    const cx=x+w/2,cy=y+h/2-6,on=e[2]==null||Math.abs(v-e[2])>.01,stp=typeof curStep!=='undefined'&&curStep>=0?curStep:Math.floor(T/beat*4)%16,ph=(T/(beat/4))%1;g.globalAlpha=on?sw:.35*sw;
    if(e[0]==='cut'){g.strokeStyle=tc;g.lineWidth=2;g.beginPath();for(let k=0;k<=w;k++){const f=k/w,yy=f<v?0:(f-v)*3,py=y+30+Math.min(50,yy*h)-Math.exp(-Math.pow((f-v)*14,2))*fxGet('res')*20;k?g.lineTo(x+k,py):g.moveTo(x+k,py);}g.stroke();}
    else if(e[0]==='res'){for(let k=0;k<4;k++){const p=((T*2+k/4)%1);g.strokeStyle=tc;g.globalAlpha=(1-p)*(.2+v);g.beginPath();g.arc(cx,cy,4+p*24*(.3+v),0,7);g.stroke();}}
    else if(e[0]==='dist'){const a=1+v*14;g.strokeStyle=COL.y;g.lineWidth=2;g.beginPath();for(let k=0;k<=w;k++){const ph=k/w*4*Math.PI-T*6,yy=Math.tanh(Math.sin(ph)*a)/Math.tanh(a);k?g.lineTo(x+k,cy-yy*22):g.moveTo(x+k,cy-yy*22);}g.stroke();}
    else if(e[0]==='dly'){const per=beat*.75,ph=(T%per)/per;for(let k=0;k<5;k++){g.globalAlpha=Math.pow(.6,k)*(v>0?1:.2)*sw;g.fillStyle=tc;g.beginPath();g.arc(x+8+(k+ph)*(w-12)/5,cy,5*Math.pow(.85,k)+1,0,7);g.fill();}}
    else if(e[0]==='rev'){for(let k=0;k<4;k++){const p=((T/(beat*2)+k/4)%1);g.globalAlpha=(1-p)*(.15+.85*v)*sw;g.strokeStyle=COL.c;g.lineWidth=2;g.beginPath();g.arc(cx,cy,3+p*(8+26*v),0,7);g.stroke();}}
    else if(e[0]==='lfd'||e[0]==='lfr'){const hz=.1*Math.pow(200,fxGet('lfr')),dep=fxGet('lfd');g.strokeStyle=e[0]==='lfd'?tc:COL.y;g.lineWidth=2;g.beginPath();for(let k=0;k<=w;k++){const yy=Math.sin((T*Math.min(6,hz)+k/w*2)*Math.PI*2)*(e[0]==='lfd'?dep:.6);k?g.lineTo(x+k,cy-yy*24):g.moveTo(x+k,cy-yy*24);}g.stroke();if(e[0]==='lfr')tx(hz.toFixed(1).replace('.',',')+' HZ',cx,y+h-8,COL.m,7,'center',800);}
    else if(e[0]==='cho'){for(let k=0;k<3;k++){g.strokeStyle=[tc,COL.c,COL.y][k];g.lineWidth=1.6;g.globalAlpha=(k?.25+.75*v:1)*sw;g.beginPath();for(let q=0;q<=w;q++){const yy=Math.sin(q/w*4*Math.PI-T*3+k*v*Math.sin(T*1.4+k)*1.6)*16;q?g.lineTo(x+q,cy-yy):g.moveTo(x+q,cy-yy);}g.stroke();}}
    else if(e[0]==='rl'){const n=e[3][fxIx(e)][0];for(let k=0;k<4;k++){const bx=x+5+k*12.5,on2=n?k<n:k===0,pp=n?(ph*n)%1:ph;g.fillStyle=on2?tc:COL.p2;g.globalAlpha=(on2?1-(n?pp*.7:0)*(k===Math.floor(ph*n)?1:.3):.4)*sw;const hh=n?28-k*4:28;g.fillRect(bx,cy+14-hh,9,hh);}if(n)tx('×'+n,cx,y+14,COL.y,9,'center',900);}
    else if(e[0]==='sk'){for(let k=0;k<8;k++){const bx=x+4+(k%4)*13,by=cy-14+Math.floor(k/4)*18,skp=((k*7+stp*3)%10)/10<v;g.fillStyle=skp?COL.p2:tc;g.globalAlpha=(skp?.5:(k===stp%8?1:.7))*sw;g.fillRect(bx,by,10,13);if(skp){g.strokeStyle=COL.r;g.lineWidth=1.5;g.beginPath();g.moveTo(bx,by);g.lineTo(bx+10,by+13);g.stroke();}}}
    else if(e[0]==='rv'){const r2=v>.5,p2=r2?1-ph:ph;g.fillStyle=COL.p2;g.fillRect(x+6,cy-2,w-12,4);g.fillStyle=r2?COL.r:tc;g.beginPath();const ax=x+6+p2*(w-12);if(r2){g.moveTo(ax-6,cy);g.lineTo(ax+4,cy-8);g.lineTo(ax+4,cy+8);}else{g.moveTo(ax+6,cy);g.lineTo(ax-4,cy-8);g.lineTo(ax-4,cy+8);}g.fill();tx(r2?'◀◀':'▶',cx,y+16,r2?COL.r:COL.m,9,'center',900);}
    else if(e[0]==='lp'){const n=e[3][fxIx(e)][0];for(let k=0;k<16;k++){const a2=n?k<n:true,cur=n?k===stp%n:k===stp;g.fillStyle=cur?COL.y:a2?tc:COL.p2;g.globalAlpha=(a2?1:.35)*sw;g.fillRect(x+4+(k%4)*12.5,cy-22+Math.floor(k/4)*11,10,9);}if(n){g.strokeStyle=COL.y;g.globalAlpha=sw;g.lineWidth=1.5;g.beginPath();g.arc(cx,y+h-14,7,T*6,T*6+5);g.stroke();}}
    else if(e[0]==='cr'){const q=v>0?Math.round(2+Math.pow(1-v,2)*14):0;g.strokeStyle=tc;g.lineWidth=2;g.beginPath();for(let k=0;k<=w;k++){let yy=Math.sin(k/w*4*Math.PI-T*4);if(q){const kk=Math.floor(k/(2+v*6))*(2+v*6);yy=Math.round(Math.sin(kk/w*4*Math.PI-T*4)*q)/q;}k?g.lineTo(x+k,cy-yy*20):g.moveTo(x+k,cy-yy*20);}g.stroke();}
    else if(e[0]==='gt'){for(let k=0;k<8;k++){const bx=x+4+k*6.5,cut2=ph>.5&&k===Math.floor(ph*8)%8;const hh=k%2&&v>0?28*(1-v):28;g.fillStyle=k%2&&v>0?COL.p2:tc;g.globalAlpha=sw*(k===Math.floor(ph*8)?1:.7);g.fillRect(bx,cy+14-hh,5,hh);}}
    else if(e[0]==='hf'){const on2=v>.5,sp=on2?T*.5:T;for(let k=0;k<2;k++){g.strokeStyle=k?COL.y:tc;g.lineWidth=2;g.beginPath();g.arc(cx+(k?14:-14),cy,9,sp*5+k,sp*5+k+4.2);g.stroke();}tx(on2?'½':'1×',cx,y+16,on2?COL.y:COL.m,10,'center',900);}
    else if(e[0]==='vol'){const hk=FXT[M.fxt||0][1],hv=hk==='r'?Math.max(...[0,1,2,3,4,5,6,7].map(r=>hit('r'+r))):hk==='m'?Math.max(...[0,1,2,3,4,5].map(t=>hit('t'+t))):hit('t'+hk),lv=v*(.35+.65*Math.max(hv,.2));for(let k=0;k<10;k++){g.fillStyle=k/10<lv?(k>7?COL.r:tc):COL.p2;g.fillRect(x+16,y+h-18-k*7,w-32,5);}}
    g.restore();g.globalAlpha=sw;tx(fit(e[1],w,8),cx,y+h+10,on?COL.t:COL.l,8,'center',800);});
  g.restore();g.globalAlpha=1;},
fxr(){const n=FXL.length,act=FXL.filter(e=>e[1]()>.005).length;tx(act?act+' EFFET(S) ACTIF(S)':'AUCUN EFFET ACTIF',8,48,act?COL.y:COL.m,9,'left',800);tx('MAINTIENS FX = TOUT COUPER',234,48,COL.m,7.5,'right',700);
  FXL.forEach((e,j)=>{const c=j%2,r=Math.floor(j/2),x=4+c*118,y=53+r*15,v=e[1](),on=v>.005;rr(x,y,114,13,3,on?'#2c241a':COL.p);tx(String(j+1),x+4,y+10,COL.l,7,'left',800);
    tx(fit(e[0],62,8),x+16,y+10,on?COL.t:COL.l,8,'left',on?800:600);rr(x+80,y+4,30,6,2,COL.p2);if(on)rr(x+80,y+4,30*Math.min(1,v),6,2,j<6?COL.a:j<9?COL.c:COL.y);});},
emidi(){const L=[...ROWS.map((n,r)=>({p:r,n,o:rOut(r),c:rCh(r),nt:rNote(r)})),...S.tracks.map((t,i)=>({p:8+i,n:t.name,o:t.mo||0,c:tCh(i)}))],ext=clkExt();
  tx('HORLOGE '+{int:'INTERNE',ext:'EXTERNE',auto:'AUTO'}[S.mcm||'int']+(ext?' · SUIT '+(CLK.bpm||S.bpm)+' BPM':S.mclk?' · ENVOYÉE AU FM-1':''),6,47,ext?COL.c:COL.m,8,'left',800);tx(MIDI.out?'FM-1 CONNECTÉ':'NON CONNECTÉ',234,47,MIDI.out?COL.c:COL.r,8,'right',800);
  const n=8,top=clamp(M.pt-3,0,L.length-n);L.slice(top,top+n).forEach((x,k)=>{const y=52+k*15.5,on=x.p===M.pt,cc=trkC(x.p);if(on)rr(2,y,236,15,3,'#24282e',COL.t,1);
    g.fillStyle=cc;g.fillRect(4,y+2,3,11);drawIcon(g,x.p<8?ROWCAT[x.p]:TYPCAT[S.tracks[x.p-8].type],10,y+1.5,12);tx(fit(x.n,60,9),26,y+11,cc,9,'left',on?800:700);
    tx(['INTERNE','FM-1','LES 2'][x.o],120,y+11,x.o?COL.y:COL.m,8,'center',800);tx('CH '+x.c,172,y+11,COL.t,9,'center',800);if(x.nt!=null)tx('N '+x.nt,232,y+11,COL.m,8,'right',700);});},
home(){const T=[...ROWS.map((n,r)=>({n,k:'r'+r,o:S.rows[r],dr:1,i:r})),...S.tracks.map((t,i)=>({n:t.name,k:'t'+i,o:t,dr:0,i:8+i})),{n:'RYTHME',g:'r'},{n:'MÉLODIE',g:'t'}];
  T.forEach((x,j)=>tile(x,3+(j%4)*59,39+Math.floor(j/4)*35,56,32));},
hryt(){ROWS.forEach((n,r)=>tile({n,k:'r'+r,o:S.rows[r],dr:1,i:r,big:1},3+(r%4)*59,40+Math.floor(r/4)*58,56,54));
  tx('🎲 CLIC SELECT · TOUT LE RYTHME : '+(S.gm.r?'COUPÉ':'ACTIF'),120,174,S.gm.r?COL.r:COL.m,8,'center',700);},
hmel(){S.tracks.forEach((t,i)=>tile({n:t.name,k:'t'+i,o:t,dr:0,i:8+i,big:1},3+(i%3)*79,40+Math.floor(i/3)*64,76,60));},
ryt(){DRAW.seq();},
mel(){lanes(38);DRAW.seq(70);},
seq(top){const cs=playing?curStep:-1;
  if(isD()){const gr=cur().g,X=40,cw=12.25;
    ROWS.forEach((n,r)=>{const Y=40+r*17,sel=r===M.pt,rc=trkC(r);if(sel)rr(1,Y-1,238,16,3,'#24282e');g.fillStyle=rc;g.fillRect(1,Y+1,2,12);tx(fit(n,32,8),5,Y+10,S.rows[r].mute?COL.l:rc,8,'left',sel?800:600);
      for(let c=0;c<16;c++){const v=gr[r][c],x=X+c*cw;rr(x,Y,cw-1.5,14,2,v?rc:(c%4?COL.p:COL.p2));if(v>1){g.fillStyle='rgba(255,255,255,.55)';g.fillRect(x+2,Y+2,cw-5.5,3);}
        if(sel&&c===M.cur)rr(x-.5,Y-.5,cw-.5,15,2,null,COL.t,1.5);}});
    if(cs>=0)rr(X+cs*cw-1,38,cw+.5,138,2,null,COL.c,1.3);return;}
  const T0=top||38,H0=176-T0,tr=trk(),L=SL(),n=2*L+1,rh=Math.min(9,Math.floor(H0/n)),X=26,cw=13,Y0=T0+(H0-n*rh)/2,pc=playing?rst():-1;
  for(let d=0;d<n;d++){const y=Y0+(n-1-d)*rh,root=d%L===0;g.fillStyle=root?'#262016':(d%2?COL.p:'#16191c');g.fillRect(X,y,cw*16,rh-1);
    if(root||rh>=8&&d%2===0)tx(degName(d),X-3,y+rh-1.5,root?COL.y:COL.m,Math.min(8,rh),'right',root?800:500);}
  for(let c=0;c<=16;c+=4){g.fillStyle=COL.l;g.fillRect(X+c*cw-.5,Y0,1,n*rh);}
  tr.steps.forEach((s,c)=>{if(s.n<0)return;const d=clamp(s.n,0,n-1),y=Y0+(n-1-d)*rh,x=X+(c+(s.o||0))*cw,w=Math.max(3,Math.min(16-c,nlen(tr,c))*cw-1.5);
    rr(x,y,w,rh-1.5,2,trkC(M.pt));if(s.a){g.fillStyle='rgba(255,255,255,.5)';g.fillRect(x+1,y+1,w-2,2);}if(s.s){g.fillStyle=COL.c;g.fillRect(x+w-3,y,3,rh-1.5);}});
  rr(X+M.cur*cw-.5,Y0-2,cw,n*rh+3,2,null,COL.t,1);
  if(pc>=0)rr(X+pc*cw-.5,Y0-2,cw,n*rh+3,2,null,COL.c,1.4);
  if(M.held.size)tx('TOUCHE DU HAUT = NOTE DU PAS',120,177,COL.c,8,'center',700);},
gryt(){tx(S.style.toUpperCase(),120,60,COL.c,20,'center',800);tx(S.bpm+' BPM · SWING '+pct(S.swing||0)+' · AUTO '+(S.auto?S.auto+' MES':'OFF'),120,75,COL.m,9,'center',600);
  const gr=cur().g;ROWS.forEach((n,r)=>{const y=84+r*11,sel=r===M.pt,rc=trkC(r);tx(fit(n,30,7),4,y+8,sel?rc:COL.l,7,'left',sel?800:600);
    for(let c=0;c<16;c++){g.globalAlpha=sel||!gr[r][c]?1:.6;rr(38+c*12.3,y,11,9,2,gr[r][c]?rc:(c%4?COL.p:COL.p2));g.globalAlpha=1;}if(sel)rr(37,y-1,197,11,2,null,COL.t,1);});
  tx(fit((S.cur+1)+' · '+cur().name+(cur().tmp?' (🎲 provisoire)':''),232,9),120,178,COL.t,9,'center',700);},
gmel(){const p=prg();if(p)p.forEach((r,k)=>{const on=playing&&k===cix,x=14+k*54;rr(x,40,50,20,5,on?COL.y:COL.p2,on?null:COL.l);tx(chName(r),x+25,54,on?'#111':COL.t,10,'center',800);});
  else tx('ACCORDS OFF · TONIQUE FIXE',120,54,COL.m,9,'center',700);
  lanes(66,1);const m=S.mels[M.mel];tx('BANQUE : '+(m?(M.mel+1)+'/'+S.mels.length+' · '+m.name:'aucune mélodie enregistrée'),120,172,COL.a,9,'center',700);},
fxa(){fxHead('FILTRE');const X=12,Y=50,W=216,H=78;rr(X,Y,W,H,4,COL.p);g.strokeStyle=COL.a;g.lineWidth=2;g.beginPath();
  const lp=S.mf,hp=S.hp||0,q=S.mq||0;for(let i=0;i<=W;i++){const f=i/W,a=f>lp?(f-lp)*4:0,b=f<hp?(hp-f)*4:0,bump=(Math.exp(-Math.pow((f-lp)*16,2))+Math.exp(-Math.pow((f-hp)*16,2))*(hp>0?1:0))*q*.5,y=Y+16+Math.min(H-20,(a+b)*H)-bump*30;i?g.lineTo(X+i,y):g.moveTo(X+i,y);}g.stroke();
  tx('COUPE-BAS',X+4,Y+H-5,COL.m,8,'left',700);tx('COUPE-HAUT',X+W-4,Y+H-5,COL.m,8,'right',700);perfMini();},
fxb(){fxHead('ESPACE');const xd=S.xd||0,xr=S.xr||0,xs=S.xs||0;
  for(let k=0;k<6;k++){const h=60*xd*Math.pow(.6,k);rr(14+k*16,118-h,10,h+1,2,COL.c);}tx('ÉCHO',44,128,COL.m,8,'center',700);
  g.fillStyle=COL.a;for(let i=0;i<60;i++){const h=50*xr*Math.exp(-i/22)*(0.6+0.4*Math.abs(Math.sin(i*1.7)));g.fillRect(112+i,118-h,1,h);}tx('REVERB',142,128,COL.m,8,'center',700);
  g.strokeStyle=COL.y;g.lineWidth=2;g.beginPath();for(let i=0;i<=40;i++){const x=i/40*2-1,a=1+xs*12,y=Math.tanh(x*a)/Math.tanh(a);i?g.lineTo(186+i,92-y*24):g.moveTo(186+i,92-y*24);}g.stroke();tx('DISTO',206,128,COL.m,8,'center',700);perfMini();},
fxd(){bars([['VOL',S.dfx.vol],['DISTO',S.dfx.dist],['DELAY',S.dfx.dly],['REVERB',S.dfx.rev]],COL.c,'TOUTE LA BATTERIE',46,80);perfMini();},
fxp(){const L=PERF.lab();const FS=['LOOP 1','LOOP 2','LOOP 4','LOOP 8','ROLL ×2','ROLL ×3','ROLL ×4','REV ◀◀','CIBLE TOUT','CIBLE RYT.','CIBLE MÉL.','VERR.','REC','REJ.','EFF.','ZÉRO'];
  FS.forEach((s,j)=>{const c=j%8,r=Math.floor(j/8),x=4+c*29.25,y=44+r*40,on=L.on(j),cc=j<8?COL.c:j===12?COL.r:COL.y;
    rr(x,y,27,34,4,on?cc:COL.p2,on?null:COL.l);const w=s.split(' ');tx(fit(w[0],25,7.5,800),x+13.5,y+15,on?'#111':COL.t,7.5,'center',800);if(w[1])tx(fit(w[1],25,7.5),x+13.5,y+26,on?'#111':COL.m,7.5,'center',700);});
  const n=Object.keys(S.am.d).length;tx(arec?'⏺ REC DES MOUVEMENTS…':n?n+' MOUVEMENT(S) EN BOUCLE · '+(S.am.on?'LECTURE':'PAUSE'):'MAINTIENS LES PADS · REC = ENREGISTRER',120,140,arec?COL.r:COL.m,9,'center',700);},
scl(){const sc=SCALES[S.scale],W=240/12;g.save();g.beginPath();g.rect(0,36,240,106);g.clip();g.translate(0,36);drawScene(g,S.scale,240,106);g.restore();
  rr(4,40,fit(S.scale.toUpperCase(),150,10).length*6+20,16,4,'rgba(0,0,0,.55)');tx(NOTES[S.root].toUpperCase()+' '+S.scale.toUpperCase(),10,52,'#fff',10,'left',800);
  for(let k=0;k<12;k++){const pc=(S.root+k)%12,inS=sc.includes(k),x=k*W;rr(x+1,144,W-2,15,2,k===0?COL.y:inS?COL.a:COL.p2);tx(NOTES[pc].toUpperCase(),x+W/2,155,k===0||inS?'#111':COL.l,7,'center',800);}
  const p=prg();tx(fit(p?p.map((r,k)=>(playing&&k===cix?'▶':'')+chName(r)).join('  ·  '):'ACCORDS OFF · SELECT = CHANGER DE GAMME',230,9),120,172,p?COL.y:COL.m,9,'center',800);},
env(){const tr=trk(),cc=trkC(M.pt),X=12,Y=44,W=216,H=92,a=tr.att||0,d=tr.dec,sus=.55,x1=X+4+a*60,x2=x1+20+d*70,x3=X+W-30;
  rr(X,Y,W,H,5,COL.p);g.strokeStyle=cc;g.lineWidth=2.5;g.beginPath();g.moveTo(X+4,Y+H-8);g.lineTo(x1,Y+10);g.quadraticCurveTo(x1+8,Y+H*sus,x2,Y+H*(1-sus)+10);g.lineTo(x3,Y+H*(1-sus)+10);g.quadraticCurveTo(x3+10,Y+H-8,X+W-4,Y+H-8);g.stroke();
  g.globalAlpha=.18;g.fillStyle=cc;g.lineTo(X+4,Y+H-8);g.fill();g.globalAlpha=1;
  const ht=HIT['t'+ti()],el=ht!=null&&ctx?ctx.currentTime-ht:9;if(el>=0&&el<1.2){const p=el/1.2,px=X+4+p*(W-8);let py;if(px<x1)py=Y+H-8-(px-X-4)/(x1-X-4)*(H-18);else if(px<x2)py=Y+10+(px-x1)/(x2-x1)*(H*(1-sus));else if(px<x3)py=Y+H*(1-sus)+10;else py=Y+H*(1-sus)+10+(px-x3)/(X+W-4-x3)*(H*sus-18);g.shadowColor=cc;g.shadowBlur=10;SK(g,px,py,4,'#fff');g.shadowBlur=0;}
  [['A',X+4+(x1-X-4)/2],['D',(x1+x2)/2],['S',(x2+x3)/2],['R',(x3+X+W)/2]].forEach(([l,x])=>tx(l,x,Y+H-12,COL.m,8,'center',800));
  tx('GLIDE',X+4,Y+H+16,COL.m,8,'left',700);const gl=tr.gl||0;g.strokeStyle=COL.y;g.lineWidth=2;g.beginPath();g.moveTo(X+40,Y+H+14);g.bezierCurveTo(X+40+gl*80,Y+H+14,X+40+gl*20,Y+H+6,X+130,Y+H+6);g.stroke();
  tx(sndName(tr),X+W-4,Y+H+16,cc,8,'right',800);},
flt(){const tr=trk(),cc=trkC(M.pt),X=10,Y=42,W=220,H=100,ty=tr.ftype||'lowpass',T=performance.now()/1000;
  const lfoN=(tr.lfd||0)*Math.sin(T*Math.PI*2*Math.min(8,lfoHz(tr)))*.12,fc=clamp(tr.cut+lfoN,0,1),q=tr.res;rr(X,Y,W,H,5,COL.p);
  g.strokeStyle='#262a30';for(let i=1;i<5;i++){g.beginPath();g.moveTo(X+i*W/5,Y);g.lineTo(X+i*W/5,Y+H);g.stroke();}
  const resp=f=>{const pk=Math.exp(-Math.pow((f-fc)*18,2))*q*.9;if(ty==='highpass')return f>fc?1+pk:Math.max(0,1-(fc-f)*5)+pk;if(ty==='bandpass')return Math.max(0,1-Math.abs(f-fc)*(4+q*10))+pk*.6;return f<fc?1+pk:Math.max(0,1-(f-fc)*5)+pk;};
  g.beginPath();for(let i=0;i<=W;i++){const y=Y+H-8-Math.min(1.9,resp(i/W))*H*.42;i?g.lineTo(X+i,y):g.moveTo(X+i,y);}g.strokeStyle=cc;g.lineWidth=2.5;g.stroke();g.lineTo(X+W,Y+H);g.lineTo(X,Y+H);g.globalAlpha=.15;g.fillStyle=cc;g.fill();g.globalAlpha=1;
  for(let i=0;i<26;i++){const f=((i*.137+T*.25)%1),a=Math.min(1,resp(f));SK(g,X+f*W,Y+H-10-((i*37)%60)*a,1.5+a,`rgba(255,255,255,${.15+a*.6})`);}
  g.strokeStyle=COL.y;g.setLineDash([3,3]);g.beginPath();g.moveTo(X+fc*W,Y+4);g.lineTo(X+fc*W,Y+H-4);g.stroke();g.setLineDash([]);
  tx(FTN[FTY.indexOf(ty)]+(tr.lfd?' · LFO ACTIF':''),120,Y+H+14,cc,9,'center',800);},
lfo(){const tr=trk(),cc=trkC(M.pt),X=10,Y=44,W=220,H=80,T=performance.now()/1000,hzv=lfoHz(tr),dep=tr.lfd||0,w=tr.lfw||0;
  const wf=ph=>{ph=((ph%1)+1)%1;return w===0?Math.sin(ph*Math.PI*2):w===1?1-4*Math.abs(ph-.5):w===2?(ph<.5?1:-1):1-2*ph;};
  rr(X,Y,W,H,5,COL.p);g.strokeStyle='#262a30';g.beginPath();g.moveTo(X,Y+H/2);g.lineTo(X+W,Y+H/2);g.stroke();
  const cyc=Math.max(1,Math.min(6,hzv*1.2)),ph0=T*hzv;g.strokeStyle=cc;g.lineWidth=2.5;g.beginPath();for(let i=0;i<=W;i++){const y=Y+H/2-wf(ph0+i/W*cyc)*(H*.4)*(.25+.75*dep);i?g.lineTo(X+i,y):g.moveTo(X+i,y);}g.stroke();
  const v=wf(ph0+cyc);g.shadowColor=cc;g.shadowBlur=12;SK(g,X+W-3,Y+H/2-v*(H*.4)*(.25+.75*dep),5,'#fff');g.shadowBlur=0;
  tx(tr.lfs?'CALÉ SUR LE TEMPO':hzv.toFixed(2).replace('.',',')+' HZ',X+6,Y+12,COL.m,8,'left',800);
  const fc=clamp(tr.cut+v*dep*.12,0,1);tx('COUPURE',X,Y+H+16,COL.m,8,'left',800);rr(X+50,Y+H+9,170,7,3,COL.p2);rr(X+50,Y+H+9,170*fc,7,3,cc);g.fillStyle='#fff';g.fillRect(X+50+170*tr.cut-1,Y+H+6,2,13);},
osc(){const tr=trk(),cc=trkC(M.pt),v=V[ti()],X=8,Y=40,W=224,H=104;rr(X,Y,W,H,5,'#0b0d10');g.strokeStyle='#1d2228';for(let i=1;i<8;i++){g.beginPath();g.moveTo(X+i*W/8,Y);g.lineTo(X+i*W/8,Y+H);g.stroke();}
  g.beginPath();g.moveTo(X,Y+H/2);g.lineTo(X+W,Y+H/2);g.stroke();
  if(v&&v.an){const n=v.an.fftSize,d=M.scope||(M.scope=new Float32Array(n));v.an.getFloatTimeDomainData(d);let z=0;for(let i=1;i<n/2;i++)if(d[i-1]<0&&d[i]>=0){z=i;break;}
    let pk=0;for(let i=0;i<n;i++)pk=Math.max(pk,Math.abs(d[i]));M.sg=Math.max(pk,(M.sg||.05)*.97);const k=pk>.0005?.42/Math.max(M.sg,.008):0;
    g.shadowColor=cc;g.shadowBlur=8;g.strokeStyle=cc;g.lineWidth=2;g.beginPath();for(let i=0;i<W;i++){const y=Y+H/2-(d[z+Math.floor(i*1.6)]||0)*H*k;i?g.lineTo(X+i,y):g.moveTo(X+i,y);}g.stroke();g.shadowBlur=0;
    if(pk<.0005)tx('JOUE UNE NOTE (TOUCHES DU HAUT) OU PLAY',120,Y+H/2-8,COL.m,8,'center',800);}
  if(tr.smp)drawIcon(g,catOfId(tr.smp),X+W-26,Y+4,20);
  tx(sndName(tr)+' · OCT '+tr.oct+' · PITCH '+((tr.po||0)>0?'+':'')+(tr.po||0),120,Y+H+14,cc,9,'center',800);},
sedit(){const b=seBuf(),id=seId(),l=S.lib.find(x=>x.id===id),cat=catOfId(id||''),cc=catC(cat);
  if(!b){tx('PAS DE SAMPLE SUR '+nameOf(M.pt),120,90,COL.m,10,'center',800);tx(id==='piano'?'LE PIANO INTERNE NE S’ÉDITE PAS':'CHOISIS UN SON DANS EDIT · SON (PAGE 1)',120,108,COL.a,9,'center',700);tx('OU ✂ SUR UN SON DE LA BIBLIOTHÈQUE',120,124,COL.m,8,'center',700);return;}
  drawIcon(g,cat,5,39,13);tx(fit(l?l.name:'son',150,10),22,49,cc,10,'left',800);tx(b.duration.toFixed(2).replace('.',',')+' s · '+(b.numberOfChannels>1?'STÉRÉO':'MONO'),235,49,COL.m,8,'right',700);
  const X=6,Y=56,W=228,H=84,d=b.getChannelData(0),sx=X+SE.st*W,ex=X+SE.en*W;rr(X,Y,W,H,4,'#0b0d10');g.fillStyle='rgba(255,255,255,.06)';g.fillRect(sx,Y,ex-sx,H);
  for(let i=0;i<W;i++){let mn=0,mx=0;const a=Math.floor(i/W*d.length),z=Math.floor((i+1)/W*d.length);for(let k=a;k<z;k+=4){const v=d[k];if(v<mn)mn=v;if(v>mx)mx=v;}const x=X+i,inS=x>=sx&&x<=ex;g.fillStyle=inS?cc:'#3a4048';g.fillRect(x,Y+H/2-mx*H*.46,1,Math.max(1,(mx-mn)*H*.46));}
  g.fillStyle=COL.c;g.fillRect(sx-1,Y,2,H);g.fillStyle=COL.a;g.fillRect(ex-1,Y,2,H);tx('◆',sx,Y+H+9,COL.c,8,'center',800);tx('◆',ex,Y+H+9,COL.a,8,'center',800);
  if(SE.src&&ctx){const el=ctx.currentTime-SE.t0,p=SE.loop?(el%SE.len):el;if(p<=SE.len){const px=X+(SE.off+p)/b.duration*W;g.fillStyle='#fff';g.fillRect(px,Y,1.5,H);}}
  tx('SÉLECTION '+((SE.en-SE.st)*b.duration).toFixed(2).replace('.',',')+' s'+(SE.undo.length?' · ↶ '+SE.undo.length:''),120,Y+H+20,COL.t,8,'center',800);},
gsty(){const n=8,top=clamp(M.sty-3,0,STY.length-n);STY.slice(top,top+n).forEach((st,k)=>{const j=top+k,y=38+k*17,on=j===M.sty,cur0=st===S.style;
    if(on)rr(2,y,112,16,3,'#24282e',COL.t,1);tx(fit((cur0?'● ':'')+st.toUpperCase(),104,9),7,y+12,cur0?COL.y:on?COL.t:COL.m,9,'left',on?800:700);});
  const st=STY[M.sty];g.save();g.beginPath();g.roundRect(118,38,120,108,5);g.clip();g.translate(118,38);g.scale(120/240,108/112);drawStyle(g,st,240,112,(108/112)/(120/240));g.restore();rr(118,38,120,108,5,null,COL.l,1);
  tx(STYLES[st].bpm+' BPM · '+(STYLES[st].sc||'Mineur').toUpperCase(),178,158,COL.t,8,'center',800);
  tx(st===S.style?'● EN COURS':'ARP MAINTENU = VALIDER',178,172,st===S.style?COL.y:COL.c,8,'center',800);tx((M.sty+1)+'/'+STY.length,58,176,COL.m,8,'center',700);},
gvis(){g.save();g.beginPath();g.rect(0,36,240,144);g.clip();g.translate(0,36);g.scale(1,144/112);drawStyle(g,S.style,240,112,144/112);g.restore();
  rr(4,40,Math.min(180,S.style.length*11+64),20,4,'rgba(0,0,0,.6)');tx(S.style.toUpperCase(),10,55,'#fff',13,'left',900);
  rr(176,40,60,20,4,'rgba(0,0,0,.6)');tx(playing?'M'+bar+' · '+(Math.floor(Math.max(0,curStep)/4)+1):'■',206,54,playing?COL.y:COL.m,10,'center',800);
  const p=prg();if(p){rr(4,152,232,20,4,'rgba(0,0,0,.6)');tx(p.map((r,k)=>(playing&&k===cix?'▶':'')+chName(r)).join('  ·  '),120,166,COL.y,9,'center',800);}},
mix(){const o=isD()?rowO():trk(),T=performance.now()/1000,beat=60/S.bpm,cc=trkC(M.pt),hk=(isD()?'r'+M.pt:'t'+ti());
  tx(isD()?ROWS[M.pt]:o.name,120,48,COL.t,10,'center',800);
  const cell=(i,l,v,fn)=>{const x=4+i*59,y=54,w=56,h=96;rr(x,y,w,h,5,COL.p);g.save();g.beginPath();g.rect(x,y,w,h);g.clip();fn(x,y,w,h,v);g.restore();
    tx(l,x+w/2,y+h+11,COL.m,8,'center',700);tx(pct(v),x+w/2,y+h+21,COL.t,8,'center',800);};
  const meter=(x,y,w,h,v)=>{const lv=v*(.35+.65*Math.max(hit(hk),.25+.25*Math.sin(T*Math.PI*2/beat)**2));for(let k=0;k<12;k++){const on=k/12<lv;g.fillStyle=on?(k>9?COL.r:k>7?COL.y:cc):COL.p2;g.fillRect(x+14,y+h-8-k*7,w-28,5);}};
  const dist=(x,y,w,h,v)=>{const a=1+v*14;g.strokeStyle=COL.y;g.lineWidth=2;g.beginPath();for(let i=0;i<=w;i++){const ph=(i/w)*4*Math.PI-T*6,yy=Math.tanh(Math.sin(ph)*a)/Math.tanh(a);i?g.lineTo(x+i,y+h/2-yy*h*.32):g.moveTo(x+i,y+h/2-yy*h*.32);}g.stroke();};
  const echo=(x,y,w,h,v)=>{const per=beat*.75,ph=(T%per)/per;for(let k=0;k<6;k++){const a=Math.pow(.62,k)*(v>0?1:0)*(.35+.65*v);const cx=x+8+(k+ph)*(w-14)/6;g.globalAlpha=Math.max(0,a*(1-ph*.3));g.fillStyle=cc;g.beginPath();g.arc(cx,y+h/2,5*Math.pow(.85,k)+2,0,7);g.fill();}g.globalAlpha=1;};
  const verb=(x,y,w,h,v)=>{const per=beat*2;for(let k=0;k<4;k++){const ph=((T/per+k/4)%1);g.globalAlpha=(1-ph)*(.2+.8*v);g.strokeStyle=COL.c;g.lineWidth=2;g.beginPath();g.arc(x+w/2,y+h/2,4+ph*(10+38*v),0,7);g.stroke();}g.globalAlpha=1;};
  const dice=(x,y,w,h,v)=>{const ar=o.ar||0,sp=ar?T*1.5:0;g.save();g.translate(x+w/2,y+h/2);g.rotate(sp);rr(-14,-14,28,28,5,ar?COL.c:COL.p2);g.fillStyle='#111';[[-6,-6],[6,6],[0,0]].forEach(([a,b])=>{g.beginPath();g.arc(a,b,2.5,0,7);g.fill();});g.restore();
    tx(ar?ar+' MES':'OFF',x+w/2,y+h-8,ar?COL.c:COL.m,8,'center',800);};
  if(isD()){cell(0,'VOLUME',o.vol,meter);cell(1,'AUTO',AUS.indexOf(o.ar||0)/6,dice);cell(2,'VOL BATT.',S.dfx.vol,meter);cell(3,'REV BATT.',S.dfx.rev,verb);}
  else{cell(0,'VOLUME',o.vol,meter);cell(1,'DISTO',o.dist,dist);cell(2,'DELAY',o.dly,echo);cell(3,'REVERB',o.rev,verb);}},
lrr(){const o=isD()?rowO():trk(),T=performance.now()/1000,sd=60/S.bpm/4,lp=o.lp||0,rl=o.rl||1,cs=playing?curStep:Math.floor(T/sd)%16;
  tx((isD()?(M.allRows?'TOUTES LES LIGNES':ROWS[M.pt]):o.name)+(o.bw?' · ◀ SENS INVERSE':''),120,48,COL.t,10,'center',800);
  // Loop : la tête de lecture tourne en boucle dans la fenêtre
  const pos=lp?Math.floor(T/sd)%lp:(o.bw?15-cs:cs);
  for(let c=0;c<16;c++){const inL=lp&&c<lp,on=c===pos;rr(8+c*14,56,12,26,3,on?(lp?COL.c:COL.a):inL?'#174a41':(c%4?COL.p:COL.p2));}
  if(lp){g.strokeStyle=COL.c;g.lineWidth=1.5;g.beginPath();const x2=8+lp*14-2,a=((T/(sd*lp))%1);g.moveTo(x2,90);g.lineTo(8,90);g.stroke();g.fillStyle=COL.c;g.beginPath();g.arc(8+a*(x2-8),90,3,0,7);g.fill();}
  tx(lp?'LOOP '+lp+' PAS':'LOOP OFF',8,108,lp?COL.c:COL.m,10,'left',800);
  // Roll : la note se répète dans le pas
  for(let k=0;k<4;k++){const act=k<rl,ph=((T/sd)*rl)%rl,lit=act&&Math.floor(ph)===k;rr(140+k*24,96,18,18,4,lit?COL.y:act?'#5a4a12':COL.p2);}
  tx(rl>1?'ROLL ×'+rl:'ROLL OFF',185,128,rl>1?COL.y:COL.m,10,'center',800);
  // Sens de lecture et reverse : flèche qui défile, onde jouée à l'envers quand on maintient
  const rv=isD()?rvR>-2:rvI===ti(),dir=o.bw?-1:1,ax=((T*60*dir)%220+220)%220;
  g.fillStyle=o.bw?COL.y:COL.m;tx(o.bw?'◀':'▶',10+ax,140,o.bw?COL.y:COL.l,14,'center',800);
  g.strokeStyle=rv?COL.r:COL.l;g.lineWidth=2;g.beginPath();for(let i=0;i<=220;i++){const e=rv?Math.pow(i/220,2):Math.pow(1-i/220,2),yy=Math.sin(i*.35+(rv?T*8:-T*8))*e;i?g.lineTo(10+i,160-yy*10):g.moveTo(10,160-yy*10);}g.stroke();
  tx(rv?'REVERSE !':'REVERSE : MAINTIENS LA TOUCHE 15',120,177,rv?COL.r:COL.m,8,'center',800);},
son(){const E=LIBP.ents(),L=M.lib,cur0=isD()?rowO().smp:trk().smp;
  const hc=L.lvl?L.f:isD()?ROWCAT[M.pt]:TYPCAT[trk().type];drawIcon(g,hc,5,39,13);tx(L.lvl?'BIBLIOTHÈQUE ▸ '+L.f.toUpperCase():'BIBLIOTHÈQUE · '+nameOf(M.pt),22,49,L.lvl?catC(L.f):COL.a,10,'left',800);tx(S.lib.length+' SONS',234,49,COL.m,8,'right',600);
  const n=7,top=clamp(L.i-3,0,Math.max(0,E.length-n));
  E.slice(top,top+n).forEach((e,k)=>{const j=top+k,y=54+k*17,on=j===L.i,isCur=e.k==='s'?e.id===cur0:e.k==='int'?!cur0:e.k==='piano'?cur0==='piano':e.f===(S.lib.find(x=>x.id===cur0)||{}).f;
    if(on)rr(3,y,234,16,3,'#2c241a',COL.a,1);
    const ec=e.k==='f'?e.f:e.k==='int'?'Interne':e.k==='piano'?'Piano':L.f,cc=catC(ec);g.fillStyle=cc;g.fillRect(4,y+2,3,12);drawIcon(g,ec,10,y+1.5,13);
    tx(fit(e.l,160,10),27,y+12,on?COL.t:e.k==='f'||e.k==='piano'?cc:COL.m,10,'left',on?800:600);
    if(e.k==='f')tx(e.n?String(e.n):'vide',230,y+12,e.n?COL.c:COL.l,9,'right',700);
    if(isCur)tx('●',e.k==='f'?212:230,y+12,COL.y,9,'right');});
  if(!E.length)tx('DOSSIER VIDE · GLISSE DES SONS SUR LE FM-1',120,100,COL.m,9,'center',700);},
sonr(){const o=isD()?rowO():trk();tx(isD()?ROWS[M.pt]:o.name,120,52,COL.t,11,'center',800);
  const l=S.lib.find(x=>x.id===o.smp);if(o.smp)drawIcon(g,catOfId(o.smp),12,60,18);tx(o.smp==='piano'?'PIANO (SON DE BASE)':l?l.name:isD()?'SON INTERNE':'SYNTHÉ INTERNE',120,74,COL.y,10,'center',700);
  const b=l&&bufs[l.id];if(b){const d=b.getChannelData(0),W=216,st=isD()?0:(o.ss||0);g.fillStyle=COL.c;for(let i=0;i<W;i++){let m=0;const a=Math.floor(i/W*d.length),z=Math.floor((i+1)/W*d.length);for(let k=a;k<z;k+=8)m=Math.max(m,Math.abs(d[k]));g.fillRect(12+i,122-m*30,1,m*60+1);}
    g.fillStyle=COL.a;g.fillRect(12+st*W,88,2,70);}
  else tx(isD()?'PAS DE SAMPLE : CHOISIS-LE DANS SON (PAGE 1)':'',120,120,COL.m,8,'center',700);},
glo(){tx(String(S.bpm),120,96,COL.y,54,'center',800);tx('BPM · SWING '+pct(S.swing||0),120,116,COL.m,10,'center',700);
  tx(S.style.toUpperCase()+' · AUTO RYTHME '+(S.auto?S.auto+' MES':'OFF'),120,140,COL.t,10,'center',700);tx('CLIC SELECT = TAP TEMPO',120,162,COL.m,8,'center',700);},
midi(){const L=[['USB',MIDI.out?(isFM(MIDI.out)?'FM-1 · ':'')+MIDI.out.name:MIDI.acc?'AUCUN APPAREIL':'NON CONNECTÉ',MIDI.out?COL.c:COL.m],
    ['HORLOGE',S.mclk?'ENVOYÉE AU FM-1':'OFF',S.mclk?COL.c:COL.m],
    ['PISTE',nameOf(M.pt)+' → '+['INTERNE','FM-1','LES DEUX'][(isD()?S.dmo:trk().mo)||0]+' · CANAL '+(isD()?dCh():tCh(ti())),COL.t],
    ['CLAVIER','LES 27 TOUCHES DU FM-1 PILOTENT CETTE VUE',COL.t],['KNOBS','PAS EN USB AVEC LE FIRMWARE ACTUEL',COL.m]];
  L.forEach(([a,b,c],k)=>{const y=56+k*22;tx(a,8,y,COL.m,8,'left',700);tx(fit(String(b),164,9),66,y,c,9,'left',700);});},
bkr(){S.rhythms.slice(0,16).forEach((r,j)=>{const c=j%4,rw=Math.floor(j/4),x=3+c*59,y=40+rw*33,on=j===S.cur,q=r===queued;
    rr(x,y,56,30,4,on?COL.y:COL.p2,q?COL.a:r.tmp?COL.l:null,q?2:1);tx(String(j+1)+(r.tmp?' 🎲':''),x+4,y+11,on?'#111':COL.m,8,'left',800);
    tx(fit(r.name.replace(/^.*· /,''),50,9),x+28,y+24,on?'#111':COL.t,9,'center',700);});
  if(S.rhythms.length>16)tx('+'+(S.rhythms.length-16)+' (ALGORITHM)',236,176,COL.m,8,'right',700);},
bkm(){if(!S.mels.length){tx('AUCUNE MÉLODIE ENREGISTRÉE',120,90,COL.m,10,'center',800);tx('CLIC SELECT = ENREGISTRER CELLE DE '+trk().name,120,110,COL.a,9,'center',700);return;}
  S.mels.slice(0,16).forEach((m,j)=>{const c=j%4,rw=Math.floor(j/4),x=3+c*59,y=40+rw*33,on=j===M.mel;
    rr(x,y,56,30,4,on?COL.a:COL.p2);tx(String(j+1),x+4,y+11,on?'#111':COL.m,8,'left',800);tx(fit(m.name,50,9),x+28,y+24,on?'#111':COL.t,9,'center',700);});},
nom(){const o=nObj();tx(M.nt?'NOM DE LA MÉLODIE':'NOM DU RYTHME '+(S.cur+1),120,54,COL.m,9,'center',700);
  if(!o)return tx('AUCUNE MÉLODIE ENREGISTRÉE',120,100,COL.m,10,'center',800);
  const nm=o.name.padEnd(24,' ').slice(0,24),cw=9.5,x0=120-12*cw;g.font='800 14px ui-monospace,Menlo,Consolas,monospace';g.textAlign='center';
  [...nm].forEach((ch,i)=>{const x=x0+i*cw,on=i===(M.np||0);if(on)rr(x,72,cw,22,2,COL.a);g.fillStyle=on?'#111':COL.t;g.fillText(ch,x+cw/2,88);});
  tx('SELECT / KNOB 3 = LETTRE · TOUCHES ◀ ▶ = POSITION',120,120,COL.m,8,'center',700);}};
function tile(x,X,Y,w,h){
  if(x.g){const off=S.gm[x.g];rr(X,Y,w,h,5,off?'#3a1414':COL.p,off?COL.r:COL.l);tx(x.n,X+w/2,Y+14,off?COL.r:COL.m,9,'center');tx(off?'COUPÉ':'ACTIF',X+w/2,Y+26,off?COL.r:COL.c,8,'center',600);return;}
  const au=aud(x.dr?'r':'t',x.dr?x.i:x.i-8),h1=hit(x.k),cc=trkC(x.i);
  rr(X,Y,w,h,5,COL.p2);if(h1&&au){g.globalAlpha=h1*.55;rr(X,Y,w,h,5,cc);g.globalAlpha=1;}
  g.fillStyle=au?cc:COL.l;g.fillRect(X,Y+4,3,h-8);const isz=w>=70?16:x.big?12:11;g.globalAlpha=au?1:.4;drawIcon(g,x.i<8?ROWCAT[x.i]:(x.o.smp==='piano'?'Piano':TYPCAT[x.o.type]),X+w-isz-3,Y+3,isz);g.globalAlpha=1;
  tx(fit(x.n,w-isz-10,x.big?10:9),X+6,Y+(x.big?15:12),au?cc:COL.l,x.big?10:9);
  if(x.o.mute){g.fillStyle=COL.l;g.fillRect(X+4,Y+(x.big?10:8),w-8,1.4);}
  g.fillStyle=COL.d;g.fillRect(X+4,Y+h-8,w-8,4);g.fillStyle=au?cc:COL.l;g.fillRect(X+4,Y+h-8,(w-8)*x.o.vol,4);
  let bx=X+4,by=Y+(x.big?30:22);if(x.o.ar){tx('A'+x.o.ar,bx,by,COL.c,7,'left',700);bx+=18;}if(!x.dr&&x.o.mo||x.dr&&S.dmo){tx('USB',bx,by,COL.y,7,'left',700);bx+=18;}
  if(x.big&&!x.dr&&x.o.smp)tx(x.o.smp==='piano'?'PIANO':'SAMPLE',bx,by,COL.pk,7,'left',700);
  if(x.big&&x.dr){const r=cur().g[x.i];for(let c=0;c<16;c++){g.fillStyle=r[c]?cc:COL.p;g.fillRect(X+4+c*((w-8)/16),Y+h-16,(w-8)/16-1,4);}}
  if(x.big&&!x.dr){const st=x.o.steps;for(let c=0;c<16;c++){g.fillStyle=st[c].n>=0?cc:COL.p;g.fillRect(X+4+c*((w-8)/16),Y+h-16,(w-8)/16-1,4);}}
  if(S.solo.includes(x.k))rr(X+.5,Y+.5,w-1,h-1,5,null,COL.y,1.5);
  if(x.i===M.pt)rr(X-1,Y-1,w+2,h+2,6,null,COL.t,2);}
function lanes(Y,big){const X=34,cw=12.5,h=big?13:5;S.tracks.forEach((t,i)=>{const y=Y+i*(h+1),sel=i===ti(),tc=trkC(8+i);
  for(let c=0;c<16;c++){const s=t.steps[c];g.globalAlpha=s.n>=0&&!sel?.55:1;g.fillStyle=s.n>=0?(t.mute?COL.l:tc):(c%4?COL.p:COL.p2);g.fillRect(X+c*cw,y,cw-1,h);g.globalAlpha=1;}
  if(sel)rr(X-1,y-1,16*cw+1,h+2,2,null,COL.t,1);tx(ab(t.name,big?8:6),X-3,y+h-1,sel?tc:COL.l,big?7.5:5.5,'right',sel?800:700);});}
const sndName=tr=>tr.smp==='piano'?'PIANO':tr.smp?'SAMPLE · '+ab((S.lib.find(x=>x.id===tr.smp)||{name:'SAMPLE'}).name,16):'SYNTHÉ · '+['SCIE','CARRÉ','TRIANGLE'][WAV.indexOf(tr.wave)];
function fxHead(t){const L=GRP.FX;tx(t,8,50,COL.a,10,'left',800);tx('SELECT = EFFET SUIVANT',232,50,COL.m,7.5,'right',700);}
function perfMini(){const L=PERF.lab();L.low.forEach((s,j)=>{const x=4+j*14.5,on=L.on(j);rr(x,148,12,10,2,on?(j<8?COL.c:j===12?COL.r:COL.y):COL.p2);});tx('PADS DU BAS : LOOP · ROLL · REVERSE · CIBLE · REC…',120,172,COL.m,7.5,'center',700);}
function bars(a,c,title,y0,hh){tx(fit(title,228,10),120,50,COL.t,10,'center',800);a.forEach(([l,v],i)=>{const x=22+i*56,h=hh||100,y=y0||58;rr(x,y,24,h,4,COL.p);rr(x,y+h*(1-clamp(v,0,1)),24,h*clamp(v,0,1),4,c);tx(l,x+12,y+h+12,COL.m,8,'center',700);});}

/* ---------- Panneau : étiquettes des touches, angle des knobs, voyants ---------- */
function panel(){if(!S)return;const P=page(),dr=isD(),L2=SL(),L=kpOn()?(dr?{low:Array.from({length:16},(_,j)=>ROWS[j%8]+(j>=8?' ACC':'')),on:()=>true,col:j=>trkC(j%8),ico:j=>ROWCAT[j%8]}:{low:WHITE.map(r=>degName(kpDeg(r))+((kpDeg(r)%L2+L2)%L2===0?'●':'')),on:j=>KP_HELD.has(WHITE[j]),col:()=>trkC(M.pt)}):P.lab();
  const up=Array.from({length:11},(_,k)=>dr?(k<8?ROWS[k]:''):kpOn()?degName(kpDeg(BLACK[k]))+((kpDeg(BLACK[k])%L2+L2)%L2===0?'●':''):degName(k+M.oct*L2)+((k+M.oct*L2)%L2===0?'●':''));
  const cs=playing?(M.page==='mel'&&!dr?rst():curStep):-1;
  const st=L.low.map((s,j)=>[s,L.on(j)?1:0,L.col?L.col(j):'',L.ico?L.ico(j):'',L.acc&&L.acc(j)?1:0,L.half&&L.half(j)?1:0,L.play&&cs===j?1:0,L.q&&L.q(j)?1:0,L.rec&&L.rec(j)?1:0]);
  const sig=M.page+'|'+M.pt+'|'+(M.kp?1:0)+'|'+up.join()+'|'+JSON.stringify(st);
  if(sig!==keySig||panelDirty){keySig=sig;
    document.querySelectorAll('#dev .ky.u').forEach((el,k)=>{el.firstChild.textContent=up[k];el.classList.toggle('dim',!up[k]);el.classList.toggle('dk',dr);el.style.setProperty('--kc',dr?(k<8?trkC(k):'#3a4048'):trkC(M.pt));});
    document.querySelectorAll('#dev .ky.l').forEach((el,j)=>{const[s,on,kc,ic,acc,half,pl,q,rc]=st[j];if(ic)el.firstChild.innerHTML=iconSVG(ic,15)+'<br>'+esc(s);else el.firstChild.textContent=s;if(kc)el.style.setProperty('--kc',kc);else el.style.removeProperty('--kc');el.className='ky l'+(on?' on':'')+(kc?' kc':'')+(acc?' ac':'')+(half&&!on?' hf':'')+(pl?' pl':'')+(q?' q':'')+(rc?' rc':'')+(s?'':' dim')+(M.lowDown.has(j)?' dn':'');});
    document.querySelectorAll('#dev .fb').forEach(el=>{const n=el.dataset.fb;el.classList.toggle('pg',M.grp===n);el.classList.toggle('pl',n==='PLAY'&&playing);el.classList.toggle('rc',n==='REC'&&(rec||arec));});
    const GL=GRP[M.grp]||[],nx=GL.length>1?PAGES[GL[(GL.indexOf(M.page)+1)%GL.length]].t:'';
    const h=$('#mhelp');if(h)h.innerHTML=`<b>${P.t}</b>${GL.length>1?` <small>page ${GL.indexOf(M.page)+1}/${GL.length} · ${M.grp} encore → ${nx}</small>`:''} · piste <i class="${dr?'cd':'cm'}">${esc(nameOf(M.pt))}</i> — touches du haut : ${dr?'jouer les 8 sons de batterie (et choisir la ligne)':'jouer '+esc(nameOf(M.pt))+' dans la gamme (OCT ±)'}`+P.help.map(([a,b])=>`<span><em>${a}</em>${b}</span>`).join('');
    panelDirty=0;}
  const ks=P.knobs(),ang=n=>-135+270*n;
  const sigK=ks.map(k=>k.get()).join()+'|'+(M.kp?1:0)+'|'+(M.fxpg||0)+'|'+(M.fxt||0)+'|'+(S.mv)+'|'+M.pt+'|'+S.cur+'|'+M.page+'|'+(S.algo||'')+'|'+S.bpm+'|'+S.mf+'|'+S.hp+'|'+S.swing;
  if(sigK!==knobSig){knobSig=sigK;const set=(n,a)=>{const el=$(`#dev [data-kn="${n}"] i`);if(el)el.style.transform=`rotate(${a}deg)`;};
    ks.forEach((k,i)=>set(KR[i],k.mx>k.mn?ang((k.get()-k.mn)/(k.mx-k.mn)):-135));
    set('MASTER',ang(S.mv==null?.8:S.mv));set('PRESETS',ang(M.pt/13));set('ALGORITHM',ang(algoAng()));const ksl=$('#dev [data-ks]');if(ksl)ksl.textContent=M.page==='hfx'?'◀ PAGE '+FXPN[M.fxpg||0]+' / D ▶':LIVEP.includes(M.page)?(M.kp?'◀ COUPURES · JEU':'COUPURES · JEU ▶'):(ALGO.find(a=>a[0]===(S.algo||'pages'))||ALGO[0])[1];}
  $('#mstat').textContent=`GROOVE · ${S.bpm} BPM · ${S.style} · ${NOTES[S.root]} ${S.scale}`+(playing?` · mesure ${bar} · pas ${curStep+1}`:' · à l’arrêt')+` · USB MIDI : ${MIDI.out?'FM‑1 connecté':'non connecté'}`;}
function loop(){if(window.VIEW==='fm1'&&S){draw();panel();}requestAnimationFrame(loop);}

/* ---------- Clavier de l'ordinateur ---------- */
document.addEventListener('keydown',e=>{if(window.VIEW!=='fm1'||!S||e.ctrlKey||e.metaKey||e.altKey)return;const t=e.target;if(/INPUT|SELECT|TEXTAREA/.test(t.tagName)&&t.type!=='range'&&t.type!=='checkbox')return;
  const c=e.code;let used=true;
  if(e.repeat){if(KC_UP.includes(c)||KC_LOW.includes(c)||KC_BTN[c])e.preventDefault();return;}
  if(KC_UP.includes(c)){const k=KC_UP.indexOf(c);upKey(k,true);flashKey('u'+k,true);}
  else if(KC_LOW.includes(c)){const j=KC_LOW.indexOf(c);lowKey(j,true);flashKey('l'+j,true);}
  else if(c==='Escape')goPage('home');
  else if(KC_BTN[c])btn(KC_BTN[c],true);
  else if(c==='ArrowRight'||c==='ArrowLeft')turn('SELECT',c==='ArrowRight'?1:-1);
  else if(c==='ArrowUp'||c==='ArrowDown')turn('PRESETS',c==='ArrowUp'?-1:1);
  else if(c==='Enter')page().click();
  else if(c==='PageUp'||c==='PageDown')turn('ALGORITHM',c==='PageDown'?1:-1);
  else if(c==='Equal'||c==='Backslash')btn(c==='Equal'?'OCT+':'OCT-',true);
  else used=false;
  if(used)e.preventDefault();});
document.addEventListener('keyup',e=>{if(window.VIEW!=='fm1'||!S)return;const c=e.code;if(KC_BTN[c]&&c!=='Escape')btn(KC_BTN[c],false);
  if(KC_UP.includes(c)){const k=KC_UP.indexOf(c);upKey(k,false);flashKey('u'+k,false);}
  else if(KC_LOW.includes(c)){const j=KC_LOW.indexOf(c);lowKey(j,false);flashKey('l'+j,false);}});
window.addEventListener('blur',()=>{if(!S)return;clearTimeout(M.hhT);M.hh=null;M.upDown.clear();liveOff();[...M.lowDown].forEach(j=>lowKey(j,false));});

/* ---------- Vues : FM-1 / Éditeur ---------- */
function edSync(){['bpm','swing','mf','hp','mq','xd','xr','xs','auto','root','scale','style'].forEach(k=>{const e=$('#'+k);if(e)e.value=S[k];});
  $('#lstyle').value=S.style;document.querySelectorAll('[data-dp]').forEach(el=>{el.value=S.dfx[el.dataset.dp];});
  renderProg();renderRhythm();renderTabs();renderRoll();renderKeys();renderMels();amUI();renderLib();midiUI();showCh();
  $('#play').textContent=playing?'■':'▶';$('#play').classList.toggle('on',playing);$('#rec').classList.toggle('on',rec);}
function setSec(x){x=x||'all';document.body.dataset.sec=x;document.querySelectorAll('main>section[data-sec]').forEach(e=>{e.hidden=x!=='all'&&e.dataset.sec!==x;});document.querySelectorAll('#secs [data-s]').forEach(b=>b.classList.toggle('on',b.dataset.s===x));M.sec=x;
  document.querySelectorAll('[data-view]').forEach(b=>b.classList.toggle('on',b.dataset.view===window.VIEW&&(window.VIEW==='fm1'||(b.dataset.sec==='all'?!SECH[x]:b.dataset.sec===x))));}
function setView(v,sec){window.VIEW=v;document.body.dataset.view=v;if(v==='ed'){setSec(sec||M.sec||'all');scrollTo(0,0);}else setSec(M.sec);
  if(v==='ed'&&S)edSync();if(v==='fm1'){M.pt=isD()?M.pt:8+S.sel;panelDirty=1;requestAnimationFrame(()=>{if(cv)sizeCanvas();});}
  history.replaceState(null,'',v==='ed'?'#'+(SECH[M.sec]||'editeur'):'#fm1');}
window.setView=setView;
function fm1Init(){fm1Build();fm1Bind();M.pt=8+(S.sel||0);
  document.querySelectorAll('[data-view]').forEach(b=>b.onclick=()=>setView(b.dataset.view,b.dataset.sec));
  document.querySelectorAll('#secs [data-s]').forEach(b=>b.onclick=()=>{setSec(b.dataset.s);history.replaceState(null,'','#'+(SECH[b.dataset.s]||'editeur'));});
  $('#bigS').onclick=()=>{document.body.classList.toggle('big');$('#bigS').classList.toggle('on');requestAnimationFrame(sizeCanvas);};
  $('#kbH').onchange=e=>{document.body.classList.toggle('nokbd',!e.target.checked);};
  window.addEventListener('resize',()=>cv&&sizeCanvas());
  // Copie des messages de l'app dans la barre du haut
  new MutationObserver(()=>{$('#tbmsg').textContent=$('#msg').textContent;}).observe($('#msg'),{childList:true,characterData:true,subtree:true});
  if(navigator.keyboard&&navigator.keyboard.getLayoutMap)navigator.keyboard.getLayoutMap().then(m=>{let ch=0;[...KC_UP,...KC_LOW,...Object.keys(KC_BTN)].forEach(c=>{const v=m.get(c);if(v&&v.trim()){KL[c]=v.toUpperCase();ch=1;}});if(ch)fm1Build();}).catch(()=>{});
  const wantLearn=/learn/.test(location.hash),h0=(location.hash.match(/rythme|melodie|bibliotheque/)||[])[0];setView(window.VIEW,h0==='bibliotheque'?'sons':h0||'all');
  if(wantLearn)setTimeout(learnStart,300);
  loop();}
if(typeof S!=='undefined'&&S)fm1Init();else window.addEventListener('groove-ready',fm1Init,{once:true});
