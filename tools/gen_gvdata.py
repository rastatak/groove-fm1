# Converts the web app's data.js (styles, scales, chords, generator rhythms) into C tables for the firmware
import re,json,sys
src=open('/home/claude/app/data.js').read()
def js_obj(name):
    i=src.index('const '+name+'=');j=src.index('\n};',i)+3;return src[i+len('const '+name+'='):j].rstrip(';\n')
def to_json(s):
    s=re.sub(r'/\*.*?\*/','',s,flags=re.S)
    s=re.sub(r"'([^']*)'",lambda m:json.dumps(m.group(1)),s)
    s=re.sub(r'([{,]\s*)([A-Za-z_][A-Za-z0-9_]*)\s*:',r'\1"\2":',s)
    s=re.sub(r',(\s*[}\]])',r'\1',s)
    return json.loads(s)
import subprocess
D=json.loads(subprocess.check_output(['node','-e',open(sys.argv[2] if len(sys.argv)>2 else '/home/claude/app/data.js').read()+';process.stdout.write(JSON.stringify({SCALES,SCP,STYLES,PROGS,RH}))']))
SCALES,SCP,STY,PROGS,RH=D['SCALES'],D['SCP'],D['STYLES'],D['PROGS'],D['RH']
sc_names=list(SCALES)
o=['/* GENERATED from the web app (data.js) by tools/gen_gvdata.py: styles, scales, chords, generator rhythms */']
o.append('#define GV_NSC %d'%len(sc_names))
o.append('static const char *const GV_SCN[GV_NSC] = {%s};'%','.join(json.dumps(n.upper()) for n in sc_names))
o.append('static const uint8_t GV_SCL[GV_NSC] = {%s};'%','.join(str(len(SCALES[n])) for n in sc_names))
o.append('static const uint8_t GV_SC[GV_NSC][7] = {%s};'%','.join('{'+','.join(map(str,SCALES[n]+[0]*(7-len(SCALES[n]))))+'}' for n in sc_names))
o.append('static const uint8_t GV_SCP[GV_NSC][5] = {%s}; /* syncopation %%, chord 3rd 5th degrees, colour notes */'%','.join('{%d,%d,%d,%d,%d}'%(round(SCP[n]['sy']*100),SCP[n]['ch'][0],SCP[n]['ch'][1],SCP[n]['co'][0],SCP[n]['co'][1]) for n in sc_names))
o.append('static const uint8_t GV_NPROG[GV_NSC] = {%s};'%','.join(str(len(PROGS[n])) for n in sc_names))
o.append('static const uint8_t GV_PROG[GV_NSC][5][4] = {%s};'%','.join('{'+','.join('{'+','.join(map(str,p))+'}' for p in PROGS[n]+[[0,0,0,0]]*(5-len(PROGS[n])))+'}' for n in sc_names))
st=list(STY)
o.append('#define GV_NSTY %d'%len(st))
o.append('static const char *const GV_STN[GV_NSTY] = {%s};'%','.join(json.dumps(n.upper()) for n in st))
o.append('static const uint8_t GV_STBPM[GV_NSTY] = {%s};'%','.join(str(STY[n]['bpm']) for n in st))
o.append('static const int8_t GV_STSC[GV_NSTY] = {%s}; /* the scale a style brings, -1 none */'%','.join(str(sc_names.index(STY[n]['sc']) if 'sc' in STY[n] else -1) for n in st))
mx=max(len(STY[n]['t']) for n in st)
o.append('#define GV_NTPL %d'%mx)
o.append('static const uint8_t GV_NTP[GV_NSTY] = {%s};'%','.join(str(len(STY[n]['t'])) for n in st))
def tpl(t):
    name,hexs=t.split('|');return name,[int(h,16) for h in hexs.split()]
o.append('static const char *const GV_TPN[GV_NSTY][GV_NTPL] = {%s};'%','.join('{'+','.join(json.dumps(tpl(t)[0]) for t in STY[n]['t'])+'}' for n in st))
o.append('static const uint16_t GV_TPL[GV_NSTY][GV_NTPL][8] = {%s}; /* bit 15 = step 1 */'%','.join('{'+','.join('{'+','.join('0x%04X'%v for v in tpl(t)[1])+'}' for t in STY[n]['t'])+'}' for n in st))
def bits(s):return int(''.join('1' if c=='x' else '0' for c in s),2)
for k in ['bass','synth']:
    for kind in ['a','b']:
        L=RH[k][kind];o.append('static const uint16_t GV_RH_%s_%s[%d] = {%s};'%(k.upper(),kind.upper(),len(L),','.join('0x%04X'%bits(s) for s in L)))
open(sys.argv[1],'w').write('\n'.join(o)+'\n')
print(len(st),'styles,',len(sc_names),'scales')
