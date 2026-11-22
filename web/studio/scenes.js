/* ---------- Scènes animées selon la gamme (écran du FM-1, page GAMME) ----------
   Chaque gamme a son décor, qui bouge en boucle et danse sur le tempo quand la lecture tourne.
   Mineur : ville la nuit sous la pluie · Phrygien : danseur égyptien devant les pyramides · Dorien : club funk et boule disco
   Mineur harmonique : charmeur de serpent dans le désert · Penta mineure : mont Fuji et cerisiers · Majeur : plage au soleil */
const SCENES={
'Mineur':(g,w,h,t,b)=>{const sky=g.createLinearGradient(0,0,0,h);sky.addColorStop(0,'#0b1020');sky.addColorStop(1,'#1c2140');g.fillStyle=sky;g.fillRect(0,0,w,h);
  g.fillStyle='#e8e6d8';g.beginPath();g.arc(w*.82,h*.2,11,0,7);g.fill();g.fillStyle='#0b1020';g.beginPath();g.arc(w*.82+5,h*.2-3,10,0,7);g.fill();
  const B=[[0,50],[22,70],[40,42],[62,82],[86,58],[108,90],[128,48],[150,74],[172,62],[196,86],[218,54]];
  B.forEach(([x,bh],i)=>{const H=bh*(h/110);g.fillStyle=i%2?'#151a2c':'#1a2036';g.fillRect(x,h-H,24,H);
    for(let yy=h-H+6;yy<h-6;yy+=9)for(let xx=x+4;xx<x+20;xx+=7){const on=Math.sin(i*7.1+xx*1.3+yy*.7+Math.floor(t*.5+b*.25))>.35;g.fillStyle=on?'#ffd23f':'#232a44';g.fillRect(xx,yy,3,4);}});
  g.strokeStyle='rgba(140,170,255,.45)';g.lineWidth=1;for(let k=0;k<45;k++){const x=(k*53.3+t*20)%w,y=(k*31.7+t*170)%h;g.beginPath();g.moveTo(x,y);g.lineTo(x-2,y+7);g.stroke();}},
'Phrygien':(g,w,h,t,b)=>{const sky=g.createLinearGradient(0,0,0,h);sky.addColorStop(0,'#ff8a1f');sky.addColorStop(.6,'#ffcf6b');sky.addColorStop(1,'#e8b45a');g.fillStyle=sky;g.fillRect(0,0,w,h);
  g.fillStyle='#fff3c4';g.beginPath();g.arc(w*.5,h*.62,22,0,7);g.fill();
  const pyr=(x,s,c)=>{g.fillStyle=c;g.beginPath();g.moveTo(x-s,h*.82);g.lineTo(x,h*.82-s*.95);g.lineTo(x+s,h*.82);g.fill();g.fillStyle='rgba(0,0,0,.18)';g.beginPath();g.moveTo(x,h*.82-s*.95);g.lineTo(x+s,h*.82);g.lineTo(x+s*.25,h*.82);g.fill();};
  pyr(w*.2,44,'#c98a3a');pyr(w*.42,30,'#b97a2e');pyr(w*.86,38,'#c98a3a');
  g.fillStyle='#d9a456';g.fillRect(0,h*.82,w,h*.18);
  // danseur : pas de côté, bras en angle qui alternent à chaque temps
  const ph=b%2,sw=Math.sin(b*Math.PI),x=w*.62+Math.sin(t*1.2)*24,y=h*.8;
  g.strokeStyle='#2a1a08';g.fillStyle='#2a1a08';g.lineWidth=3;g.lineCap='round';
  g.beginPath();g.moveTo(x,y-30);g.lineTo(x,y-12);g.stroke();
  g.beginPath();g.moveTo(x,y-12);g.lineTo(x-6+sw*3,y);g.moveTo(x,y-12);g.lineTo(x+6+sw*3,y);g.stroke();
  const a=ph<1?1:-1;g.beginPath();g.moveTo(x,y-27);g.lineTo(x+9*a,y-27);g.lineTo(x+9*a,y-37);g.moveTo(x,y-24);g.lineTo(x-9*a,y-24);g.lineTo(x-9*a,y-16);g.stroke();
  g.beginPath();g.arc(x,y-35,4.5,0,7);g.fill();
  g.fillStyle='#1d4fbf';g.beginPath();g.moveTo(x-5,y-38);g.lineTo(x+5,y-38);g.lineTo(x+6,y-30);g.lineTo(x-6,y-30);g.fill();g.fillStyle='#ffd23f';g.fillRect(x-5,y-36,10,1.5);
  g.fillStyle='rgba(42,26,8,.35)';g.beginPath();g.ellipse(x,y+1,9,2,0,0,7);g.fill();},
'Dorien':(g,w,h,t,b)=>{g.fillStyle='#160b26';g.fillRect(0,0,w,h);
  for(let k=0;k<5;k++){const a=Math.sin(t*.9+k*1.3)*.5;g.save();g.translate(w*(.1+k*.2),0);g.rotate(a);const gr=g.createLinearGradient(0,0,0,h);gr.addColorStop(0,['rgba(255,138,31,.45)','rgba(53,227,196,.4)','rgba(255,179,220,.4)'][k%3]);gr.addColorStop(1,'rgba(0,0,0,0)');g.fillStyle=gr;g.beginPath();g.moveTo(-3,0);g.lineTo(3,0);g.lineTo(26,h);g.lineTo(-26,h);g.fill();g.restore();}
  const cx=w/2,cy=22,r=15;g.strokeStyle='#888';g.beginPath();g.moveTo(cx,0);g.lineTo(cx,cy-r);g.stroke();
  for(let i=-3;i<=3;i++)for(let j=-3;j<=3;j++){const x=cx+i*4.3,y=cy+j*4.3;if((i*4.3)**2+(j*4.3)**2>r*r)continue;const v=Math.sin(i*2.1+j*1.7+t*6)>.3;g.fillStyle=v?'#ffffff':'#8a94a6';g.fillRect(x-2,y-2,3.6,3.6);}
  for(let k=0;k<12;k++){const a=k/12*7+t*.8;g.fillStyle='rgba(255,255,255,.8)';g.fillRect(cx+Math.cos(a)*w*.42,cy+40+Math.sin(a*1.3)*h*.3,2,2);}
  g.fillStyle='#2b1846';g.fillRect(0,h*.86,w,h*.14);for(let i=0;i<8;i++){g.fillStyle=(i+Math.floor(b))%2?'#ff8a1f':'#35e3c4';g.globalAlpha=.55;g.fillRect(i*w/8,h*.86,w/8-2,h*.14);g.globalAlpha=1;}
  [[.3,0],[.7,.5]].forEach(([fx,o])=>{const x=w*fx,y=h*.86,s=Math.sin((b+o)*Math.PI);g.strokeStyle='#0a0510';g.fillStyle='#0a0510';g.lineWidth=4;g.lineCap='round';
    g.beginPath();g.moveTo(x,y-32+Math.abs(s)*3);g.lineTo(x+s*3,y-14);g.lineTo(x-6,y);g.moveTo(x+s*3,y-14);g.lineTo(x+7,y);g.moveTo(x,y-28);g.lineTo(x-10,y-40+s*8);g.moveTo(x,y-28);g.lineTo(x+11,y-22-s*8);g.stroke();
    g.beginPath();g.arc(x,y-37+Math.abs(s)*3,5,0,7);g.fill();g.beginPath();g.arc(x,y-39+Math.abs(s)*3,8,Math.PI,0);g.fill();});},
'Mineur harmonique':(g,w,h,t,b)=>{const sky=g.createLinearGradient(0,0,0,h);sky.addColorStop(0,'#1b1238');sky.addColorStop(1,'#6b3a5a');g.fillStyle=sky;g.fillRect(0,0,w,h);
  for(let k=0;k<30;k++){g.fillStyle='rgba(255,255,255,'+(.4+.6*Math.abs(Math.sin(t+k)))+')';g.fillRect((k*67)%w,(k*29)%(h*.55),1.5,1.5);}
  g.fillStyle='#ffe9a6';g.beginPath();g.arc(w*.18,h*.22,10,0,7);g.fill();g.fillStyle='#1b1238';g.beginPath();g.arc(w*.18+4,h*.22-2,9,0,7);g.fill();
  g.fillStyle='#3a2140';g.beginPath();g.moveTo(0,h*.8);for(let x=0;x<=w;x+=10)g.lineTo(x,h*.78+Math.sin(x*.05)*5);g.lineTo(w,h);g.lineTo(0,h);g.fill();
  // charmeur et serpent qui ondule sur le tempo
  const x=w*.62,y=h*.86;g.fillStyle='#8a5a2a';g.beginPath();g.moveTo(x-12,y-16);g.lineTo(x+12,y-16);g.lineTo(x+9,y);g.lineTo(x-9,y);g.fill();g.strokeStyle='#5a3a18';g.lineWidth=1;for(let k=0;k<3;k++){g.beginPath();g.moveTo(x-11,y-12+k*4);g.lineTo(x+11,y-12+k*4);g.stroke();}
  g.strokeStyle='#35e3c4';g.lineWidth=4;g.lineCap='round';g.beginPath();for(let k=0;k<=20;k++){const yy=y-16-k*2.4,xx=x+Math.sin(k*.45-t*4-b*Math.PI)*(3+k*.35);k?g.lineTo(xx,yy):g.moveTo(xx,yy);}g.stroke();
  const hy=y-16-48,hx=x+Math.sin(20*.45-t*4-b*Math.PI)*10;g.fillStyle='#35e3c4';g.beginPath();g.ellipse(hx,hy,6,4,0,0,7);g.fill();g.fillStyle='#111';g.fillRect(hx+1,hy-1.5,1.5,1.5);
  const mx=w*.3,my=h*.86;g.fillStyle='#120a1e';g.beginPath();g.arc(mx,my-20,6,0,7);g.fill();g.beginPath();g.ellipse(mx,my-8,10,9,0,Math.PI,0);g.fill();g.fillStyle='#ff8a1f';g.beginPath();g.ellipse(mx,my-26,7,3,0,0,7);g.fill();
  g.strokeStyle='#c9a46a';g.lineWidth=2;g.beginPath();g.moveTo(mx+4,my-18);g.lineTo(mx+20,my-14+Math.sin(t*8)*1);g.stroke();
  for(let k=0;k<3;k++){const p=((t*.8+k/3)%1);g.fillStyle='rgba(255,210,63,'+(1-p)+')';g.font='9px sans-serif';g.fillText('♪',mx+22+p*30,my-16-p*24);}},
'Penta mineure':(g,w,h,t,b)=>{const sky=g.createLinearGradient(0,0,0,h);sky.addColorStop(0,'#ffb3dc');sky.addColorStop(1,'#ffe9d6');g.fillStyle=sky;g.fillRect(0,0,w,h);
  g.fillStyle='#ff4646';g.beginPath();g.arc(w*.72,h*.3,14,0,7);g.fill();
  g.fillStyle='#5b6b8c';g.beginPath();g.moveTo(w*.08,h*.82);g.lineTo(w*.4,h*.28);g.lineTo(w*.48,h*.28);g.lineTo(w*.82,h*.82);g.fill();
  g.fillStyle='#ffffff';g.beginPath();g.moveTo(w*.33,h*.4);g.lineTo(w*.4,h*.28);g.lineTo(w*.48,h*.28);g.lineTo(w*.56,h*.42);g.lineTo(w*.5,h*.38);g.lineTo(w*.45,h*.43);g.lineTo(w*.4,h*.38);g.fill();
  g.fillStyle='#7da36a';g.fillRect(0,h*.82,w,h*.18);
  const tx0=w*.84,ty=h*.82;g.fillStyle='#d62828';g.fillRect(tx0-14,ty-34,4,34);g.fillRect(tx0+10,ty-34,4,34);g.fillRect(tx0-20,ty-38,40,4);g.fillRect(tx0-16,ty-28,32,3);
  g.fillStyle='#5a3a28';g.fillRect(w*.06,ty-30,4,30);g.fillStyle='#ffb3dc';[[0,-34],[-8,-28],[8,-28],[-4,-38],[5,-40]].forEach(([a,c])=>{g.beginPath();g.arc(w*.06+2+a,ty+c,7,0,7);g.fill();});
  for(let k=0;k<18;k++){const p=(t*.12+k/18)%1,x=(k*41+Math.sin(t+k)*14)%w,y=p*h;g.fillStyle='#ff8fc6';g.save();g.translate(x,y);g.rotate(t*2+k);g.beginPath();g.ellipse(0,0,2.6,1.4,0,0,7);g.fill();g.restore();}
  const ly=ty-46+Math.sin(b*Math.PI)*1.5;g.strokeStyle='#5a3a28';g.lineWidth=1;g.beginPath();g.moveTo(w*.6,ty-58);g.lineTo(w*.6,ly-6);g.stroke();g.fillStyle='#ff8a1f';g.beginPath();g.ellipse(w*.6,ly,5,7,0,0,7);g.fill();},
'Majeur':(g,w,h,t,b)=>{const sky=g.createLinearGradient(0,0,0,h);sky.addColorStop(0,'#5ec8ff');sky.addColorStop(1,'#bfeaff');g.fillStyle=sky;g.fillRect(0,0,w,h);
  const sx=w*.78,sy=h*.24;g.save();g.translate(sx,sy);g.rotate(t*.6);g.strokeStyle='#ffd23f';g.lineWidth=3;for(let k=0;k<12;k++){g.rotate(Math.PI/6);g.beginPath();g.moveTo(18+Math.sin(b*Math.PI)*2,0);g.lineTo(26+Math.sin(b*Math.PI)*3,0);g.stroke();}g.restore();
  g.fillStyle='#ffd23f';g.beginPath();g.arc(sx,sy,14,0,7);g.fill();g.fillStyle='#111';g.fillRect(sx-7,sy-3,5,3);g.fillRect(sx+2,sy-3,5,3);g.fillRect(sx-2,sy-2,4,1);
  g.strokeStyle='#111';g.lineWidth=1.2;g.beginPath();g.arc(sx,sy+2,6,.2,Math.PI-.2);g.stroke();
  g.fillStyle='#1f8fd6';g.beginPath();g.moveTo(0,h*.62);for(let x=0;x<=w;x+=6)g.lineTo(x,h*.62+Math.sin(x*.06+t*2)*3);g.lineTo(w,h*.78);g.lineTo(0,h*.78);g.fill();
  g.fillStyle='#ffffff';for(let x=0;x<w;x+=16){g.fillRect((x+t*30)%w,h*.62+Math.sin(x*.06+t*2)*3-1,6,1.5);}
  g.fillStyle='#ffe3a0';g.fillRect(0,h*.76,w,h*.24);
  const px=w*.2,py=h*.82,sw=Math.sin(t*1.4+b*.5)*4;g.strokeStyle='#8a5a2a';g.lineWidth=4;g.beginPath();g.moveTo(px,py);g.quadraticCurveTo(px+4,py-30,px+8+sw,py-56);g.stroke();
  g.fillStyle='#2f9e44';for(let k=0;k<5;k++){const a=-2.6+k*.75+Math.sin(t*1.4+k)*.08;g.save();g.translate(px+8+sw,py-56);g.rotate(a);g.beginPath();g.ellipse(14,0,15,4,0,0,7);g.fill();g.restore();}
  for(let k=0;k<2;k++){const x=((t*14+k*120)%(w+40))-20;g.fillStyle='#fff';g.beginPath();g.arc(x,h*.16+k*10,6,0,7);g.arc(x+7,h*.16+k*10-3,7,0,7);g.arc(x+14,h*.16+k*10,6,0,7);g.fill();}}
};

/* ---------- La mascotte A4E : le petit monstre orange à un œil, casque sur la tête, qui danse sur le tempo ----------
   (x, y) = ses pieds, s = échelle (1 ≈ 135 px de haut), b = temps musicaux, k = impulsion du temps, mv = sa danse */
const MNS={o:'#f08030',od:'#b8561c',ol:'#ff9d4f',ink:'#111',ir:'#7cc242',ird:'#3f8f2a',pk:'#e98aa6',hp:'#8a0d1e',hpl:'#c8283a',band:'#555'};
/* où il danse dans chaque scène : [x, y des pieds, taille] en fraction de la scène (trance : au centre de la lumière) */
const MONPOS={trance:[.5,.93,.66]};
const MONMV={house:'slide',psytrance:'spin',techno:'bob',tekno:'jump',schranz:'bob',minimal:'bob',hardcore:'jump',hardtek:'jump',acidcore:'jump',
  tribe:'jump',trance:'arms',dub:'sway',acid:'wave',trap:'bounce',drill:'bounce'};
function drawMonster(g,x,y,s,b,k,mv,ar){ar=ar||1; /* ar : l'étirement vertical de la scène, compensé */
  const P=Math.PI,ph=b%1,sb=Math.sin(P*b),c2=Math.sin(2*P*b);mv=mv||'bob';
  let hop=0,tilt=0,sq=k*.08,armL=-.5,armR=-.5,wv=0,look=0;        /* bras : angle (0 = le long du corps, -P/2 = à l'horizontale) */
  if(mv==='jump'){hop=Math.max(0,Math.sin(2*P*ph))*20;sq=k*.18-(hop>12?.06:0);armL=armR=-2.6+Math.sin(2*P*ph)*.3;}
  else if(mv==='bounce'){hop=Math.abs(Math.sin(2*P*b))*7;tilt=sb*.08;armL=-1.2-Math.abs(c2)*.6;armR=-.3;}
  else if(mv==='sway'){tilt=sb*.16;hop=Math.abs(sb)*3;armL=-.4-Math.max(0,sb)*1.2;armR=-.4-Math.max(0,-sb)*1.2;look=sb*3;}
  else if(mv==='arms'){hop=Math.abs(sb)*5;tilt=sb*.05;armL=-2.7+Math.sin(2*P*b)*.35;armR=-2.7-Math.sin(2*P*b)*.35;}
  else if(mv==='wave'){tilt=Math.sin(P*b*.5)*.1;hop=Math.abs(sb)*5;armR=-2.5;wv=Math.sin(4*P*b)*.45;armL=-.35;}
  else if(mv==='slide'){x+=Math.sin(P*b*.5)*16*s;tilt=-Math.cos(P*b*.5)*.1;hop=Math.abs(sb)*3;armR=-1.55+Math.sin(2*P*b)*.15;armL=-.25;look=-Math.cos(P*b*.5)*4;}   /* il glisse en pointant le doigt */
  else if(mv==='spin'){const sp=(b%8)>7;tilt=sp?((b%8)-7)*2*P:Math.sin(P*b)*.08;hop=Math.abs(sb)*6+(sp?10:0);armL=armR=sp?-1.6:-1-Math.abs(c2)*.8;}
  else{hop=Math.abs(sb)*4;tilt=Math.sin(2*P*b)*.04;armR=-2.3+Math.abs(sb)*.6;wv=Math.sin(2*P*b)*.3;armL=-.4-k*.3;}   /* bob : il hoche la tête et lève le bras sur le temps */
  g.save();g.translate(x,y);g.scale(s,s/ar);
  g.fillStyle='rgba(0,0,0,.32)';g.beginPath();g.ellipse(0,0,36-hop*.7,5,0,0,7);g.fill();
  g.translate(0,-hop);if(Math.abs(tilt)>.3){g.translate(0,-65);g.rotate(tilt);g.translate(0,65);}else g.rotate(tilt);g.scale(1+sq,1-sq);
  const ink=MNS.ink,o=MNS.o;
  /* 5 tentacules : les 3 du milieu sont ses jambes (elles piétinent à tour de rôle), les 2 des bords ses mains */
  const legs=[-1,0,1].map(i=>{const ord=[0,2,1][i+1],p2=(b*2+ord/3)%1,lift=hop>4?3+Math.abs(i)*2:Math.max(0,Math.sin(2*P*p2))*8,
    tx=i*17+Math.sin(P*b+i)*1.5;return{bx:i*11,by:-38,cx:i*15,cy:-22,tx,ty:-4-lift-(i?2:0)};});
  const cap=(x0,y0,cx,cy,x1,y1,w)=>{g.lineWidth=w;g.lineCap='round';g.beginPath();g.moveTo(x0,y0);g.quadraticCurveTo(cx,cy,x1,y1);g.stroke();};
  /* les bras : épaule -> coude -> main ronde */
  const arm=(d,a,w2)=>{const sx=d*25,sy=-44,L=38,ax=sx+d*Math.sin(-a)*L*.55,ay=sy+Math.cos(a)*L*.55,hx=sx+d*Math.sin(-a+(a<-1.8?w2:0))*L,hy=sy+Math.cos(a+(a<-1.8?w2:0))*L;
    return{sx,sy,ax:(sx+hx)/2+d*5,ay:(sy+hy)/2+4,hx,hy};};
  const AL=arm(-1,armL,-wv),AR=arm(1,armR,wv);
  const body=()=>{g.beginPath();g.moveTo(-30,-92);g.bezierCurveTo(-31,-138,31,-138,30,-92);g.bezierCurveTo(30,-72,32,-56,31,-46);g.quadraticCurveTo(30,-35,20,-35);
    g.lineTo(-20,-35);g.quadraticCurveTo(-30,-35,-31,-46);g.bezierCurveTo(-32,-56,-30,-72,-30,-92);g.closePath();};
  /* contour noir (une passe), puis l'orange */
  g.strokeStyle=ink;g.lineJoin='round';legs.forEach(l=>cap(l.bx,l.by,l.cx,l.cy,l.tx,l.ty,18));
  body();g.lineWidth=5;g.stroke();
  g.strokeStyle=o;g.fillStyle=o;legs.forEach(l=>cap(l.bx,l.by,l.cx,l.cy,l.tx,l.ty,13));
  body();g.fill();
  /* volume : ombre à droite, reflet en haut à gauche, creux entre les tentacules */
  g.save();body();g.clip();g.fillStyle=MNS.od;g.globalAlpha=.75;g.beginPath();g.ellipse(32,-80,11,58,-.08,0,7);g.fill();
  g.fillStyle=MNS.ol;g.globalAlpha=.55;g.beginPath();g.ellipse(-15,-118,6,11,-.5,0,7);g.fill();g.restore();
  g.strokeStyle=ink;g.lineWidth=2;for(let j=0;j<2;j++){const xm=(legs[j].bx+legs[j+1].bx)/2;g.beginPath();g.moveTo(xm,-35);g.lineTo((legs[j].cx+legs[j+1].cx)/2,-27);g.stroke();}
  /* les bras, devant le corps : contour puis orange, la main ronde */
  [AL,AR].forEach(A=>{g.strokeStyle=ink;cap(A.sx,A.sy,A.ax,A.ay,A.hx,A.hy,17);
    g.strokeStyle=o;cap(A.sx-(A.sx>0?5:-5),A.sy-1,A.ax,A.ay,A.hx,A.hy,12);
    const mx=(A.ax*2+A.hx)/3,my=(A.ay*2+A.hy)/3;g.strokeStyle=ink;g.lineWidth=1.2;g.beginPath();g.arc(mx,my,4,0,P*.9);g.stroke();});
  /* l'œil : grand, blanc cerclé de noir, iris vert ; il cligne et regarde où il danse */
  const bl=(b%6)>5.85?.12:1,ey=-100,lx=look+Math.sin(P*b*.25)*1.5;
  g.fillStyle=ink;g.beginPath();g.ellipse(0,ey,19.5,19.5*bl+1,0,0,7);g.fill();g.fillStyle='#fff';g.beginPath();g.ellipse(0,ey,16.5,16.5*bl,0,0,7);g.fill();
  if(bl>.5){g.fillStyle=MNS.ird;g.beginPath();g.arc(lx,ey+1,12,0,7);g.fill();g.fillStyle=MNS.ir;g.beginPath();g.arc(lx,ey+1,10.2,0,7);g.fill();
    g.strokeStyle=MNS.ird;g.lineWidth=.7;for(let r=0;r<14;r++){const a2=r/14*2*P;g.beginPath();g.moveTo(lx+Math.cos(a2)*6,ey+1+Math.sin(a2)*6);g.lineTo(lx+Math.cos(a2)*10,ey+1+Math.sin(a2)*10);g.stroke();}
    g.fillStyle=ink;g.beginPath();g.arc(lx,ey+1,5.6+k*1.3,0,7);g.fill();g.fillStyle='#fff';g.beginPath();g.ellipse(lx+3.2,ey-2.5,2.6,3.2,.6,0,7);g.fill();g.beginPath();g.arc(lx-3,ey+4,1.1,0,7);g.fill();}
  /* le grand sourire : il s'ouvre sur le temps, deux crocs en haut, la langue rouge */
  const mo=15+k*7,my=-74;g.fillStyle=ink;g.beginPath();g.moveTo(-21,my);g.quadraticCurveTo(0,my-4,21,my);g.quadraticCurveTo(19,my+mo,0,my+mo);g.quadraticCurveTo(-19,my+mo,-21,my);g.fill();
  g.fillStyle='#fff';g.beginPath();g.moveTo(-10,my-1);g.lineTo(-6,my-1);g.lineTo(-8,my+4.5);g.fill();g.beginPath();g.moveTo(6,my-1);g.lineTo(10,my-1);g.lineTo(8,my+4.5);g.fill();
  g.fillStyle='#d8202c';g.beginPath();g.ellipse(0,my+mo-3,10,5+k*1.5,0,P,0);g.fill();g.fillStyle='#ff6f7a';g.beginPath();g.ellipse(-3,my+mo-5,2.2,1,0,0,7);g.fill();
  /* le casque */
  g.strokeStyle=ink;g.lineWidth=7;g.beginPath();g.arc(0,-104,35,P*1.04,P*1.96);g.stroke();g.strokeStyle=MNS.band;g.lineWidth=4;g.stroke();
  [-1,1].forEach(d=>{g.fillStyle=ink;g.beginPath();g.roundRect(d*34-7,-112,14,27,7);g.fill();g.fillStyle=MNS.hp;g.beginPath();g.roundRect(d*34-5.5,-110.5,11,24,5.5);g.fill();
    g.fillStyle=MNS.hpl;g.beginPath();g.roundRect(d*34-(d>0?1:4),-107,4,17,2);g.fill();
    if(k>.4){g.strokeStyle='rgba(255,210,63,'+(k*.8)+')';g.lineWidth=1.6;for(let r=1;r<3;r++){g.beginPath();g.arc(d*34,-98,8+r*5,d>0?-.6:P-.6,d>0?.6:P+.6);g.stroke();}}});
  g.restore();}
function drawScene(g,scale,w,h){const t=performance.now()/1000,beat=60/(S.bpm||120),b=playing?t/beat:t*.5;(SCENES[scale]||SCENES['Mineur'])(g,w,h,t,b);
  if(S.mascot!==false)drawMonster(g,w*.84,h-3,h*.6/140,b,playing?Math.max(0,1-(b%1)*3):0,MONMV[S.style]||'bob');}

/* ---------- Scènes animées selon le style (page ARP · STYLES) ----------
   t = secondes, b = temps musicaux (avance sur le tempo pendant la lecture), k = impulsion du temps (1 sur le temps, puis retombe) */
const SK=(g,x,y,s,c)=>{g.fillStyle=c;g.beginPath();g.arc(x,y,s,0,7);g.fill();};
const smiley=(g,x,y,r,melt,t)=>{g.fillStyle='#ffd23f';g.beginPath();g.arc(x,y,r,0,7);g.fill();if(melt){for(let i=0;i<5;i++){const dx=x-r*.8+i*r*.4,l=(Math.sin(t*2+i)+1.4)*r*.35;g.fillRect(dx-2,y+r*.6,4,l);SK(g,dx,y+r*.6+l,2.5,'#ffd23f');}}
  g.fillStyle='#111';g.beginPath();g.ellipse(x-r*.33,y-r*.25,r*.1,r*.2,0,0,7);g.ellipse(x+r*.33,y-r*.25,r*.1,r*.2,0,0,7);g.fill();g.strokeStyle='#111';g.lineWidth=r*.1;g.beginPath();g.arc(x,y+r*.05,r*.55,.25,Math.PI-.25);g.stroke();};
const fig=(g,x,y,s,ph,c,arms)=>{g.strokeStyle=c;g.fillStyle=c;g.lineWidth=s*.22;g.lineCap='round';const a=Math.sin(ph);
  g.beginPath();g.moveTo(x,y-s*2);g.lineTo(x,y-s);g.moveTo(x,y-s);g.lineTo(x-s*.5+a*s*.2,y);g.moveTo(x,y-s);g.lineTo(x+s*.5+a*s*.2,y);
  if(arms==='up'){g.moveTo(x,y-s*1.8);g.lineTo(x-s*.6,y-s*2.8-a*s*.3);g.moveTo(x,y-s*1.8);g.lineTo(x+s*.6,y-s*2.8+a*s*.3);}else{g.moveTo(x,y-s*1.8);g.lineTo(x-s*.7,y-s*1.3+a*s*.6);g.moveTo(x,y-s*1.8);g.lineTo(x+s*.7,y-s*1.3-a*s*.6);}
  g.stroke();g.beginPath();g.arc(x,y-s*2.35,s*.35,0,7);g.fill();};
const speaker=(g,x,y,w,h,k)=>{g.fillStyle='#1a1a1e';g.fillRect(x,y,w,h);g.strokeStyle='#333';g.strokeRect(x+.5,y+.5,w-1,h-1);const r=Math.min(w,h)*.36;
  g.fillStyle='#2a2a30';g.beginPath();g.arc(x+w/2,y+h/2,r*(1+k*.12),0,7);g.fill();g.fillStyle='#0c0c0e';g.beginPath();g.arc(x+w/2,y+h/2,r*.45*(1+k*.2),0,7);g.fill();};
const STYLESC={
techno:(g,w,h,t,b,k)=>{g.fillStyle='#0b0b0f';g.fillRect(0,0,w,h);
  for(let i=0;i<6;i++){g.fillStyle='#16161c';g.fillRect(10+i*40,0,10,h);}
  g.strokeStyle='rgba(255,40,40,.7)';g.lineWidth=1.2;for(let i=0;i<7;i++){const a=Math.sin(t*.7+i)*.9;g.beginPath();g.moveTo(w/2,8);g.lineTo(w/2+Math.sin(a)*w,h);g.stroke();}
  for(let i=0;i<14;i++){g.fillStyle='rgba(120,120,140,.12)';g.beginPath();g.arc((i*37+t*8)%w,h*.75+Math.sin(t+i)*6,18,0,7);g.fill();}
  for(let i=0;i<12;i++)fig(g,10+i*20,h,7,b*Math.PI+i,'#000','down');
  if(k>.6){g.fillStyle='rgba(255,255,255,'+(k-.6)*1.6+')';g.fillRect(0,0,w,h);}},
tekno:(g,w,h,t,b,k)=>{g.fillStyle='#07101e';g.fillRect(0,0,w,h);for(let i=0;i<25;i++)SK(g,(i*53)%w,(i*29)%(h*.5),.8,'#fff');
  g.fillStyle='#0d1a0d';g.fillRect(0,h*.82,w,h*.18);g.fillStyle='#222';g.fillRect(w*.62,h*.5,w*.34,h*.34);g.fillRect(w*.88,h*.6,w*.1,h*.24);SK(g,w*.68,h*.86,5,'#111');SK(g,w*.9,h*.86,5,'#111');
  for(let c=0;c<3;c++)for(let r=0;r<3;r++)speaker(g,12+c*34,h*.18+r*24,32,22,k);
  for(let i=0;i<3;i++){const p=(t*1.5+i/3)%1;g.strokeStyle='rgba(53,227,196,'+(1-p)*.5+')';g.beginPath();g.arc(62,h*.5,40+p*80,-.6,.6);g.stroke();}
  for(let i=0;i<6;i++)fig(g,140+i*14,h*.86,5,b*Math.PI*2+i,'#05070a','up');},
trance:(g,w,h,t,b,k)=>{const gr=g.createRadialGradient(w/2,h*.6,5,w/2,h*.6,w*.7);gr.addColorStop(0,'#fff6d8');gr.addColorStop(.3,'#7ab8ff');gr.addColorStop(1,'#1a1050');g.fillStyle=gr;g.fillRect(0,0,w,h);
  g.save();g.translate(w/2,h*.6);g.rotate(t*.15);for(let i=0;i<16;i++){g.rotate(Math.PI/8);g.fillStyle='rgba(255,255,255,'+(.08+.08*k)+')';g.beginPath();g.moveTo(0,0);g.lineTo(w,-8);g.lineTo(w,8);g.fill();}g.restore();
  for(let i=0;i<14;i++)fig(g,8+i*17.5,h+2,6,b*Math.PI+i*.5,'#0b0830',Math.sin(b*Math.PI*.5+i)>0?'up':'down');},
psytrance:(g,w,h,t,b,k)=>{g.fillStyle='#090014';g.fillRect(0,0,w,h);g.save();g.translate(w/2,h/2);
  for(let r=6;r>0;r--){g.save();g.rotate(t*(r%2?.4:-.3)+b*.05);const n=6+r*2;for(let i=0;i<n;i++){g.rotate(Math.PI*2/n);g.fillStyle=['#ff00c8','#00ffd5','#fff200','#7b2cff','#ff6a00','#00ff6a'][(r+i)%6];g.globalAlpha=.75;g.beginPath();g.ellipse(r*9*(1+k*.1),0,r*3.2,r*1.4,0,0,7);g.fill();}g.restore();}
  g.globalAlpha=1;SK(g,0,0,5+k*3,'#fff');g.restore();},
acidcore:(g,w,h,t,b,k)=>{g.fillStyle='#062b06';g.fillRect(0,0,w,h);for(let y=0;y<h;y+=4){g.fillStyle='rgba(60,255,60,'+(.05+.05*Math.sin(y*.3+t*8))+')';g.fillRect(0,y,w,2);}
  const gl=k>.5?(Math.random()*8-4):0;smiley(g,w/2+gl,h*.42,30+k*4,1,t);
  g.strokeStyle='#7dff3a';g.lineWidth=2;g.beginPath();for(let x=0;x<w;x++){const y=h*.9-Math.sin(x*.12+t*10)*Math.exp(-((x/w*4+t)%4))*12;x?g.lineTo(x,y):g.moveTo(x,y);}g.stroke();},
minimal:(g,w,h,t,b,k)=>{g.fillStyle='#f2f0ea';g.fillRect(0,0,w,h);g.strokeStyle='#ddd';for(let x=0;x<w;x+=20){g.beginPath();g.moveTo(x,0);g.lineTo(x,h);g.stroke();}
  for(let i=0;i<8;i++){const ph=b*Math.PI*.5+i*.5,x=20+i*28,len=40+i*4,a=Math.sin(ph)*.6;g.strokeStyle='#222';g.lineWidth=1;g.beginPath();g.moveTo(x,6);g.lineTo(x+Math.sin(a)*len,6+Math.cos(a)*len);g.stroke();SK(g,x+Math.sin(a)*len,6+Math.cos(a)*len,4,i===Math.floor(b)%8?'#ff4646':'#111');}
  g.fillStyle='#111';g.fillRect(0,h-6,w*((b%4)/4),2);},
dub:(g,w,h,t,b,k)=>{const sky=g.createLinearGradient(0,0,0,h);sky.addColorStop(0,'#ff9a3c');sky.addColorStop(1,'#5a1e3a');g.fillStyle=sky;g.fillRect(0,0,w,h);SK(g,w*.7,h*.45,20,'#ffe08a');
  g.fillStyle='#2a0f1e';g.fillRect(0,h*.8,w,h*.2);speaker(g,18,h*.3,60,64,k);speaker(g,82,h*.5,40,40,k);
  for(let i=0;i<5;i++){const p=((t*.6+i/5)%1);g.strokeStyle='rgba(255,240,200,'+(1-p)*.7+')';g.lineWidth=2;g.beginPath();g.arc(48,h*.3+32,30+p*120,-.8,.8);g.stroke();}
  const px=w*.86;g.strokeStyle='#2a0f1e';g.lineWidth=4;g.beginPath();g.moveTo(px,h*.8);g.quadraticCurveTo(px+4,h*.5,px-6+Math.sin(t)*3,h*.3);g.stroke();g.fillStyle='#2a0f1e';for(let i=0;i<5;i++){g.save();g.translate(px-6+Math.sin(t)*3,h*.3);g.rotate(-2.6+i*.7);g.beginPath();g.ellipse(13,0,14,3.5,0,0,7);g.fill();g.restore();}},
'hip-hop':(g,w,h,t,b,k)=>{g.fillStyle='#7a3b2a';g.fillRect(0,0,w,h);g.strokeStyle='rgba(0,0,0,.25)';for(let y=0;y<h;y+=10){g.beginPath();g.moveTo(0,y);g.lineTo(w,y);g.stroke();for(let x=(y/10%2)*15;x<w;x+=30){g.beginPath();g.moveTo(x,y);g.lineTo(x,y+10);g.stroke();}}
  g.font='900 30px Impact,Arial Black,sans-serif';g.fillStyle='#ff6fb5';g.strokeStyle='#111';g.lineWidth=3;g.save();g.translate(w/2,h*.3);g.rotate(-.08);g.strokeText('GROOVE',-58,10);g.fillText('GROOVE',-58,10);g.restore();
  const bx=w/2-60,by=h*.5+k*-3;g.fillStyle='#d9d9d9';g.fillRect(bx,by,120,46);g.fillStyle='#333';g.fillRect(bx+40,by-10,40,10);speaker(g,bx+4,by+4,38,38,k);speaker(g,bx+78,by+4,38,38,k);
  for(let i=0;i<6;i++){const v=Math.abs(Math.sin(t*6+i*1.3))*(.4+k*.6);g.fillStyle=i<4?'#35e3c4':'#ffd23f';g.fillRect(bx+46+i*5,by+38-v*30,4,v*30);}},
trap:(g,w,h,t,b,k)=>{g.fillStyle='#14051f';g.fillRect(0,0,w,h);for(let i=0;i<8;i++){g.fillStyle='rgba(160,60,255,.12)';g.beginPath();g.arc((i*61+t*12)%w,h*.7,40,0,7);g.fill();}
  for(let r=0;r<4;r++)for(let c=0;c<4;c++){const on=(Math.floor(b*2)+r*3+c*5)%7===0;rr0(g,w/2-58+c*30,12+r*24,26,20,on?'#c86bff':'#2a1440');}
  const rl=Math.floor(t*16)%8;for(let i=0;i<8;i++){g.fillStyle=i<=rl?'#ffd23f':'#3a2a10';g.fillRect(16+i*8,h-14,6,8);}
  g.strokeStyle='#ff4646';g.lineWidth=2.5;g.beginPath();for(let x=0;x<70;x++){const y=h-30+Math.sin(x*.25-t*8)*6*Math.exp(-x/40);x?g.lineTo(w-80+x,y):g.moveTo(w-80,y);}g.stroke();},
drill:(g,w,h,t,b,k)=>{g.fillStyle='#14171c';g.fillRect(0,0,w,h);[[0,40],[26,60],[48,35],[70,70],[96,50],[120,82],[146,44],[170,66],[196,56],[218,74]].forEach(([x,bh])=>{g.fillStyle='#1d2229';g.fillRect(x,h-bh,24,bh);for(let y=h-bh+5;y<h-5;y+=8)for(let xx=x+3;xx<x+20;xx+=6){g.fillStyle=Math.sin(x+y+xx)>.6?'#ffd27a':'#262c35';g.fillRect(xx,y,3,3);}});
  g.strokeStyle='rgba(180,190,210,.35)';for(let i=0;i<40;i++){const x=(i*47+t*30)%w,y=(i*29+t*200)%h;g.beginPath();g.moveTo(x,y);g.lineTo(x-3,y+8);g.stroke();}
  g.strokeStyle='#8fd3ff';g.lineWidth=3;g.beginPath();const sl=(b%2)/2;for(let x=0;x<=w;x+=4){const y=h*.35-(x/w<sl?Math.pow(x/w/sl,2):1)*18+Math.sin(x*.1+t*4)*2;x?g.lineTo(x,y):g.moveTo(x,y);}g.stroke();},
schranz:(g,w,h,t,b,k)=>{g.fillStyle='#1a1410';g.fillRect(0,0,w,h);g.fillStyle='#3a3028';g.fillRect(0,h*.78,w,h*.22);
  g.fillStyle='#555';g.fillRect(w/2-34,h*.62,68,14);g.fillRect(w/2-20,h*.72,40,10);
  const ph=b%1,ang=ph<.2?ph/.2*1.2:1.2-(ph-.2)/.8*1.2;g.save();g.translate(w/2+50,h*.6);g.rotate(-ang);g.fillStyle='#6b4a2a';g.fillRect(-60,-3,60,6);g.fillStyle='#888';g.fillRect(-72,-12,18,24);g.restore();
  if(ph<.25){for(let i=0;i<18;i++){const a=Math.random()*Math.PI,r=Math.random()*40*(1-ph*3);SK(g,w/2-10+Math.cos(a)*r,h*.62-Math.sin(a)*r,1.5,['#ffd23f','#ff8a1f','#fff'][i%3]);}g.fillStyle='rgba(255,180,80,'+(.25-ph)+')';g.fillRect(0,0,w,h);}},
acid:(g,w,h,t,b,k)=>{g.fillStyle='#020b04';g.fillRect(0,0,w,h);g.strokeStyle='rgba(60,255,100,.12)';for(let x=0;x<w;x+=20){g.beginPath();g.moveTo(x,0);g.lineTo(x,h);g.stroke();}for(let y=0;y<h;y+=20){g.beginPath();g.moveTo(0,y);g.lineTo(w,y);g.stroke();}
  const cut=.5+.45*Math.sin(t*.8),res=.6+.4*Math.sin(t*.37);g.strokeStyle='#5dff7a';g.lineWidth=2;g.shadowColor='#5dff7a';g.shadowBlur=8;g.beginPath();
  for(let x=0;x<=w;x++){const ph=(x/w*4+t*2)%1,saw=1-2*ph,ring=Math.sin(ph*Math.PI*2*(3+cut*8))*Math.exp(-ph*4)*res;const y=h/2-(saw*.35*cut+ring*.6)*h*.4*(1+k*.2);x?g.lineTo(x,y):g.moveTo(x,y);}g.stroke();g.shadowBlur=0;
  smiley(g,w-24,22,12,0,t);g.fillStyle='#5dff7a';g.font='700 8px monospace';g.fillText('CUT '+Math.round(cut*100)+'  RES '+Math.round(res*100),6,h-6);},
'acid house':(g,w,h,t,b,k)=>{g.fillStyle='#120018';g.fillRect(0,0,w,h);
  for(let r=0;r<6;r++)for(let c=-6;c<7;c++){const y0=h*.55+r*r*2.2+r*4,y1=h*.55+(r+1)*(r+1)*2.2+(r+1)*4,sx=1+r*.5;g.fillStyle=(r+c+Math.floor(b))%2?'#fff':'#111';g.beginPath();g.moveTo(w/2+c*16*sx,y0);g.lineTo(w/2+(c+1)*16*sx,y0);g.lineTo(w/2+(c+1)*16*(sx+.5),y1);g.lineTo(w/2+c*16*(sx+.5),y1);g.fill();}
  for(let i=0;i<5;i++){g.strokeStyle=['#00ff9c','#ff00c8','#00b3ff','#ffd23f','#ff4646'][i];g.lineWidth=1.2;const a=t*1.3+i*1.2;g.beginPath();g.moveTo(w/2,10);g.lineTo(w/2+Math.cos(a)*w,h*.55);g.stroke();}
  g.save();g.translate(w/2,h*.3);g.rotate(Math.sin(t*2)*.3);smiley(g,0,0,18+k*3,0,t);g.restore();},
house:(g,w,h,t,b,k)=>{g.fillStyle='#1b1022';g.fillRect(0,0,w,h);for(let i=0;i<4;i++){const a=Math.sin(t*.8+i*1.6);g.fillStyle=['rgba(255,138,31,.25)','rgba(53,227,196,.22)','rgba(255,111,181,.22)','rgba(255,210,63,.2)'][i];g.beginPath();g.moveTo(30+i*60,0);g.lineTo(30+i*60+a*60-30,h);g.lineTo(30+i*60+a*60+30,h);g.fill();}
  const cx=w*.42,cy=h*.55,r=h*.36;g.fillStyle='#2b2b30';g.fillRect(cx-r-12,cy-r-10,2*r+80,2*r+20);g.save();g.translate(cx,cy);g.rotate(t*3.5);g.fillStyle='#0a0a0a';g.beginPath();g.arc(0,0,r,0,7);g.fill();
  g.strokeStyle='#1f1f22';for(let i=3;i<r;i+=3){g.beginPath();g.arc(0,0,i,0,7);g.stroke();}g.fillStyle='#ff8a1f';g.beginPath();g.arc(0,0,r*.3,0,7);g.fill();g.fillStyle='#fff';g.fillRect(-1,-r*.28,2,r*.15);g.restore();
  g.strokeStyle='#bbb';g.lineWidth=3;g.beginPath();g.moveTo(cx+r+40,cy-r+4);g.lineTo(cx+r+30,cy+6);g.lineTo(cx+r*.55,cy+r*.35);g.stroke();
  for(let i=0;i<5;i++){const v=Math.abs(Math.sin(t*5+i))*(.5+k*.5);g.fillStyle='#35e3c4';g.fillRect(cx+r+24+i*6,cy+r-4-v*24,4,v*24);}},
hardcore:(g,w,h,t,b,k)=>{const sh=k*6;g.save();g.translate((Math.random()-.5)*sh,(Math.random()-.5)*sh);g.fillStyle=k>.5?'#3a0000':'#0a0000';g.fillRect(-10,-10,w+20,h+20);
  for(let i=0;i<10;i++){g.strokeStyle='rgba(255,40,40,'+(.3+.4*k)+')';g.lineWidth=2;g.beginPath();g.moveTo(0,i*h/10+Math.sin(t*20+i)*3*k);for(let x=0;x<w;x+=8)g.lineTo(x,i*h/10+(Math.random()-.5)*6*k);g.stroke();}
  const r=26+k*14;g.fillStyle='#ff2a2a';g.beginPath();g.arc(w/2,h/2,r,0,7);g.fill();g.fillStyle='#111';g.beginPath();g.arc(w/2,h/2,r*.5,0,7);g.fill();
  g.font='900 22px Impact,Arial Black,sans-serif';g.fillStyle='#fff';g.textAlign='center';g.fillText(Math.floor(b)%2?'BOUM':'BAM',w/2,h/2+8);g.textAlign='left';g.restore();},
hardtek:(g,w,h,t,b,k)=>{g.fillStyle='#000';g.fillRect(0,0,w,h);const cx=w/2,cy=h/2;
  for(let i=0;i<70;i++){const a=i*2.4,z=((i*.37+t*1.4)%1),r=z*z*w*.8;g.strokeStyle=`hsla(${(i*12+t*80)%360},100%,60%,${z})`;g.lineWidth=1+z*2;g.beginPath();g.moveTo(cx+Math.cos(a)*r*.6,cy+Math.sin(a)*r*.6);g.lineTo(cx+Math.cos(a)*r,cy+Math.sin(a)*r);g.stroke();}
  for(let i=0;i<5;i++){const z=((t*1.1+i/5)%1);g.strokeStyle='rgba(255,138,31,'+z*.8+')';g.lineWidth=2;g.strokeRect(cx-z*w/2,cy-z*h/2,z*w,z*h);}
  SK(g,cx,cy,4+k*10,'rgba(255,255,255,'+(.5+k*.5)+')');},
tribe:(g,w,h,t,b,k)=>{g.fillStyle='#0a0614';g.fillRect(0,0,w,h);for(let i=0;i<30;i++)SK(g,(i*71)%w,(i*23)%(h*.5),.9,'#fff');g.fillStyle='#120c08';g.fillRect(0,h*.82,w,h*.18);
  const fx=w/2,fy=h*.82;for(let i=0;i<14;i++){const p=(t*1.2+i/14)%1;SK(g,fx+Math.sin(i*3+t*2)*6*p,fy-p*44,(1-p)*7,i%2?'#ff8a1f':'#ffd23f');}
  g.fillStyle='rgba(255,140,40,'+(.12+.08*k)+')';g.beginPath();g.arc(fx,fy,60,0,7);g.fill();
  [-70,-38,38,70].forEach((dx,i)=>fig(g,fx+dx+Math.sin(t*.8+i)*3,fy,7,b*Math.PI+i,'#000',i%2?'up':'down'));
  [[-96,0],[96,0]].forEach(([dx])=>{g.fillStyle='#4a2a14';g.fillRect(fx+dx-8,fy-16,16,16);g.fillStyle='#c9a46a';g.beginPath();g.ellipse(fx+dx,fy-16-k*2,8,2.5,0,0,7);g.fill();});}
};
const rr0=(g,x,y,w,h,c)=>{g.fillStyle=c;g.beginPath();g.roundRect(x,y,w,h,3);g.fill();};
function drawStyle(g,st,w,h,ar){const t=performance.now()/1000,beat=60/(STYLES[st]&&STYLES[st].bpm||S.bpm||120),b=playing?t/(60/S.bpm):t/beat,k=Math.max(0,1-(b%1)*3);
  (STYLESC[st]||STYLESC.techno)(g,w,h,t,b,k);
  if(S.mascot!==false){const mp=MONPOS[st]||[.86,1,.6];drawMonster(g,w*mp[0],h*mp[1]-3,h*mp[2]/140,b,k,MONMV[st]||'bob',ar);}}
