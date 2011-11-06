#include "gens.h"
#ifdef GENS_DEBUG

#include <stdio.h>
#include <string.h>

#include "mem_z80.h"

char Mnemonics[256][16] =
{
  "NOP","LD BC,#h","LD (BC),A","INC BC","INC B","DEC B","LD B,*h","RLCA",
  "EX AF,AF'","ADD HL,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*h","RRCA",
  "DJNZ *h","LD DE,#h","LD (DE),A","INC DE","INC D","DEC D","LD D,*h","RLA",
  "JR *h","ADD HL,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*h","RRA",
  "JR NZ,*h","LD HL,#h","LD (#h),HL","INC HL","INC H","DEC H","LD H,*h","DAA",
  "JR Z,*h","ADD HL,HL","LD HL,(#h)","DEC HL","INC L","DEC L","LD L,*h","CPL",
  "JR NC,*h","LD SP,#h","LD (#h),A","INC SP","INC (HL)","DEC (HL)","LD (HL),*h","SCF",
  "JR C,*h","ADD HL,SP","LD A,(#h)","DEC SP","INC A","DEC A","LD A,*h","CCF",
  "LD B,B","LD B,C","LD B,D","LD B,E","LD B,H","LD B,L","LD B,(HL)","LD B,A",
  "LD C,B","LD C,C","LD C,D","LD C,E","LD C,H","LD C,L","LD C,(HL)","LD C,A",
  "LD D,B","LD D,C","LD D,D","LD D,E","LD D,H","LD D,L","LD D,(HL)","LD D,A",
  "LD E,B","LD E,C","LD E,D","LD E,E","LD E,H","LD E,L","LD E,(HL)","LD E,A",
  "LD H,B","LD H,C","LD H,D","LD H,E","LD H,H","LD H,L","LD H,(HL)","LD H,A",
  "LD L,B","LD L,C","LD L,D","LD L,E","LD L,H","LD L,L","LD L,(HL)","LD L,A",
  "LD (HL),B","LD (HL),C","LD (HL),D","LD (HL),E","LD (HL),H","LD (HL),L","HALT","LD (HL),A",
  "LD A,B","LD A,C","LD A,D","LD A,E","LD A,H","LD A,L","LD A,(HL)","LD A,A",
  "ADD B","ADD C","ADD D","ADD E","ADD H","ADD L","ADD (HL)","ADD A",
  "ADC B","ADC C","ADC D","ADC E","ADC H","ADC L","ADC (HL)","ADC,A",
  "SUB B","SUB C","SUB D","SUB E","SUB H","SUB L","SUB (HL)","SUB A",
  "SBC B","SBC C","SBC D","SBC E","SBC H","SBC L","SBC (HL)","SBC A",
  "AND B","AND C","AND D","AND E","AND H","AND L","AND (HL)","AND A",
  "XOR B","XOR C","XOR D","XOR E","XOR H","XOR L","XOR (HL)","XOR A",
  "OR B","OR C","OR D","OR E","OR H","OR L","OR (HL)","OR A",
  "CP B","CP C","CP D","CP E","CP H","CP L","CP (HL)","CP A",
  "RET NZ","POP BC","JP NZ,#h","JP #h","CALL NZ,#h","PUSH BC","ADD *h","RST 00h",
  "RET Z","RET","JP Z,#h","PFX_CB","CALL Z,#h","CALL #h","ADC *h","RST 08h",
  "RET NC","POP DE","JP NC,#h","OUTA (*h)","CALL NC,#h","PUSH DE","SUB *h","RST 10h",
  "RET C","EXX","JP C,#h","INA (*h)","CALL C,#h","PFX_DD","SBC *h","RST 18h",
  "RET PO","POP HL","JP PO,#h","EX HL,(SP)","CALL PO,#h","PUSH HL","AND *h","RST 20h",
  "RET PE","LD PC,HL","JP PE,#h","EX DE,HL","CALL PE,#h","PFX_ED","XOR *h","RST 28h",
  "RET P","POP AF","JP P,#h","DI","CALL P,#h","PUSH AF","OR *h","RST 30h",
  "RET M","LD SP,HL","JP M,#h","EI","CALL M,#h","PFX_FD","CP *h","RST 38h"
};


char MnemonicsCB[256][16] =
{
  "RLC B","RLC C","RLC D","RLC E","RLC H","RLC L","RLC xHL","RLC A",
  "RRC B","RRC C","RRC D","RRC E","RRC H","RRC L","RRC xHL","RRC A",
  "RL B","RL C","RL D","RL E","RL H","RL L","RL xHL","RL A",
  "RR B","RR C","RR D","RR E","RR H","RR L","RR xHL","RR A",
  "SLA B","SLA C","SLA D","SLA E","SLA H","SLA L","SLA xHL","SLA A",
  "SRA B","SRA C","SRA D","SRA E","SRA H","SRA L","SRA xHL","SRA A",
  "SLL B","SLL C","SLL D","SLL E","SLL H","SLL L","SLL xHL","SLL A",
  "SRL B","SRL C","SRL D","SRL E","SRL H","SRL L","SRL xHL","SRL A",
  "BIT 0,B","BIT 0,C","BIT 0,D","BIT 0,E","BIT 0,H","BIT 0,L","BIT 0,(HL)","BIT 0,A",
  "BIT 1,B","BIT 1,C","BIT 1,D","BIT 1,E","BIT 1,H","BIT 1,L","BIT 1,(HL)","BIT 1,A",
  "BIT 2,B","BIT 2,C","BIT 2,D","BIT 2,E","BIT 2,H","BIT 2,L","BIT 2,(HL)","BIT 2,A",
  "BIT 3,B","BIT 3,C","BIT 3,D","BIT 3,E","BIT 3,H","BIT 3,L","BIT 3,(HL)","BIT 3,A",
  "BIT 4,B","BIT 4,C","BIT 4,D","BIT 4,E","BIT 4,H","BIT 4,L","BIT 4,(HL)","BIT 4,A",
  "BIT 5,B","BIT 5,C","BIT 5,D","BIT 5,E","BIT 5,H","BIT 5,L","BIT 5,(HL)","BIT 5,A",
  "BIT 6,B","BIT 6,C","BIT 6,D","BIT 6,E","BIT 6,H","BIT 6,L","BIT 6,(HL)","BIT 6,A",
  "BIT 7,B","BIT 7,C","BIT 7,D","BIT 7,E","BIT 7,H","BIT 7,L","BIT 7,(HL)","BIT 7,A",
  "RES 0,B","RES 0,C","RES 0,D","RES 0,E","RES 0,H","RES 0,L","RES 0,(HL)","RES 0,A",
  "RES 1,B","RES 1,C","RES 1,D","RES 1,E","RES 1,H","RES 1,L","RES 1,(HL)","RES 1,A",
  "RES 2,B","RES 2,C","RES 2,D","RES 2,E","RES 2,H","RES 2,L","RES 2,(HL)","RES 2,A",
  "RES 3,B","RES 3,C","RES 3,D","RES 3,E","RES 3,H","RES 3,L","RES 3,(HL)","RES 3,A",
  "RES 4,B","RES 4,C","RES 4,D","RES 4,E","RES 4,H","RES 4,L","RES 4,(HL)","RES 4,A",
  "RES 5,B","RES 5,C","RES 5,D","RES 5,E","RES 5,H","RES 5,L","RES 5,(HL)","RES 5,A",
  "RES 6,B","RES 6,C","RES 6,D","RES 6,E","RES 6,H","RES 6,L","RES 6,(HL)","RES 6,A",
  "RES 7,B","RES 7,C","RES 7,D","RES 7,E","RES 7,H","RES 7,L","RES 7,(HL)","RES 7,A",
  "SET 0,B","SET 0,C","SET 0,D","SET 0,E","SET 0,H","SET 0,L","SET 0,(HL)","SET 0,A",
  "SET 1,B","SET 1,C","SET 1,D","SET 1,E","SET 1,H","SET 1,L","SET 1,(HL)","SET 1,A",
  "SET 2,B","SET 2,C","SET 2,D","SET 2,E","SET 2,H","SET 2,L","SET 2,(HL)","SET 2,A",
  "SET 3,B","SET 3,C","SET 3,D","SET 3,E","SET 3,H","SET 3,L","SET 3,(HL)","SET 3,A",
  "SET 4,B","SET 4,C","SET 4,D","SET 4,E","SET 4,H","SET 4,L","SET 4,(HL)","SET 4,A",
  "SET 5,B","SET 5,C","SET 5,D","SET 5,E","SET 5,H","SET 5,L","SET 5,(HL)","SET 5,A",
  "SET 6,B","SET 6,C","SET 6,D","SET 6,E","SET 6,H","SET 6,L","SET 6,(HL)","SET 6,A",
  "SET 7,B","SET 7,C","SET 7,D","SET 7,E","SET 7,H","SET 7,L","SET 7,(HL)","SET 7,A"
};


char MnemonicsED[256][16] =
{
  "DB ED,00","DB ED,01","DB ED,02","DB ED,03","DB ED,04","DB ED,05","DB ED,06","DB ED,07",
  "DB ED,08","DB ED,09","DB ED,0A","DB ED,0B","DB ED,0C","DB ED,0D","DB ED,0E","DB ED,0F",
  "DB ED,10","DB ED,11","DB ED,12","DB ED,13","DB ED,14","DB ED,15","DB ED,16","DB ED,17",
  "DB ED,18","DB ED,19","DB ED,1A","DB ED,1B","DB ED,1C","DB ED,1D","DB ED,1E","DB ED,1F",
  "DB ED,20","DB ED,21","DB ED,22","DB ED,23","DB ED,24","DB ED,25","DB ED,26","DB ED,27",
  "DB ED,28","DB ED,29","DB ED,2A","DB ED,2B","DB ED,2C","DB ED,2D","DB ED,2E","DB ED,2F",
  "DB ED,30","DB ED,31","DB ED,32","DB ED,33","DB ED,34","DB ED,35","DB ED,36","DB ED,37",
  "DB ED,38","DB ED,39","DB ED,3A","DB ED,3B","DB ED,3C","DB ED,3D","DB ED,3E","DB ED,3F",
  "IN B,(C)","OUT (C),B","SBC HL,BC","LD (#h),BC","NEG","RETN","IM 0","LD I,A",
  "IN C,(C)","OUT (C),C","ADC HL,BC","LD BC,(#h)","NEG","RETI","IM 0","LD R,A",
  "IN D,(C)","OUT (C),D","SBC HL,DE","LD (#h),DE","NEG","RETN","IM 1","LD A,I",
  "IN E,(C)","OUT (C),E","ADC HL,DE","LD DE,(#h)","NEG","RETN","IM 2","LD A,R",
  "IN H,(C)","OUT (C),H","SBC HL,HL","LD (#h),HL","NEG","RETN","IM 0","RRD",
  "IN L,(C)","OUT (C),L","ADC HL,HL","LD HL,(#h)","NEG","RETN","IM 0","RLD",
  "IN F,(C)","OUT (C),0","SBC HL,SP","LD (#h),SP","NEG","RETN","IM 1","NOP",
  "IN A,(C)","OUT (C),A","ADC HL,SP","LD SP,(#h)","NEG","RETN","IM 2","NOP",
  "DB ED,80","DB ED,81","DB ED,82","DB ED,83","DB ED,84","DB ED,85","DB ED,86","DB ED,87",
  "DB ED,88","DB ED,89","DB ED,8A","DB ED,8B","DB ED,8C","DB ED,8D","DB ED,8E","DB ED,8F",
  "DB ED,90","DB ED,91","DB ED,92","DB ED,93","DB ED,94","DB ED,95","DB ED,96","DB ED,97",
  "DB ED,98","DB ED,99","DB ED,9A","DB ED,9B","DB ED,9C","DB ED,9D","DB ED,9E","DB ED,9F",
  "LDI","CPI","INI","OUTI","DB ED,A4","DB ED,A5","DB ED,A6","DB ED,A7",
  "LDD","CPD","IND","OUTD","DB ED,AC","DB ED,AD","DB ED,AE","DB ED,AF",
  "LDIR","CPIR","INIR","OTIR","DB ED,B4","DB ED,B5","DB ED,B6","DB ED,B7",
  "LDDR","CPDR","INDR","OTDR","DB ED,BC","DB ED,BD","DB ED,BE","DB ED,BF",
  "DB ED,C0","DB ED,C1","DB ED,C2","DB ED,C3","DB ED,C4","DB ED,C5","DB ED,C6","DB ED,C7",
  "DB ED,C8","DB ED,C9","DB ED,CA","DB ED,CB","DB ED,CC","DB ED,CD","DB ED,CE","DB ED,CF",
  "DB ED,D0","DB ED,D1","DB ED,D2","DB ED,D3","DB ED,D4","DB ED,D5","DB ED,D6","DB ED,D7",
  "DB ED,D8","DB ED,D9","DB ED,DA","DB ED,DB","DB ED,DC","DB ED,DD","DB ED,DE","DB ED,DF",
  "DB ED,E0","DB ED,E1","DB ED,E2","DB ED,E3","DB ED,E4","DB ED,E5","DB ED,E6","DB ED,E7",
  "DB ED,E8","DB ED,E9","DB ED,EA","DB ED,EB","DB ED,EC","DB ED,ED","DB ED,EE","DB ED,EF",
  "DB ED,F0","DB ED,F1","DB ED,F2","DB ED,F3","DB ED,F4","DB ED,F5","DB ED,F6","DB ED,F7",
  "DB ED,F8","DB ED,F9","DB ED,FA","DB ED,FB","DB ED,FC","DB ED,FD","DB ED,FE","DB ED,FF"
};


char MnemonicsXX[256][16] =
{
  "NOP","LD BC,#h","LD (BC),A","INC BC","INC B","DEC B","LD B,*h","RLCA",
  "EX AF,AF'","ADD I%,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*h","RRCA",
  "DJNZ *h","LD DE,#h","LD (DE),A","INC DE","INC D","DEC D","LD D,*h","RLA",
  "JR *h","ADD I%,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*h","RRA",
  "JR NZ,*h","LD I%,#h","LD (#h),I%","INC I%","INC I%h","DEC I%h","LD I%Xh,*h","DAA",
  "JR Z,*h","ADD I%,I%","LD I%,(#h)","DEC I%","INC I%l","DEC I%l","LD I%l,*h","CPL",
  "JR NC,*h","LD SP,#h","LD (#h),A","INC SP","INC (I%+*h)","DEC (I%+*h)","LD (I%+*h),*h","SCF",
  "JR C,*h","ADD I%,SP","LD A,(#h)","DEC SP","INC A","DEC A","LD A,*h","CCF",
  "LD B,B","LD B,C","LD B,D","LD B,E","LD B,I%h","LD B,I%l","LD B,(I%+*h)","LD B,A",
  "LD C,B","LD C,C","LD C,D","LD C,E","LD C,I%h","LD C,I%l","LD C,(I%+*h)","LD C,A",
  "LD D,B","LD D,C","LD D,D","LD D,E","LD D,I%h","LD D,I%l","LD D,(I%+*h)","LD D,A",
  "LD E,B","LD E,C","LD E,D","LD E,E","LD E,I%h","LD E,I%l","LD E,(I%+*h)","LD E,A",
  "LD I%h,B","LD I%h,C","LD I%h,D","LD I%h,E","LD I%h,I%h","LD I%h,I%l","LD H,(I%+*h)","LD I%h,A",
  "LD I%l,B","LD I%l,C","LD I%l,D","LD I%l,E","LD I%l,I%h","LD I%l,I%l","LD L,(I%+*h)","LD I%l,A",
  "LD (I%+*h),B","LD (I%+*h),C","LD (I%+*h),D","LD (I%+*h),E","LD (I%+*h),H","LD (I%+*h),L","HALT","LD (I%+*h),A",
  "LD A,B","LD A,C","LD A,D","LD A,E","LD A,I%h","LD A,L","LD A,(I%+*h)","LD A,A",
  "ADD B","ADD C","ADD D","ADD E","ADD I%h","ADD I%l","ADD (I%+*h)","ADD A",
  "ADC B","ADC C","ADC D","ADC E","ADC I%h","ADC I%l","ADC (I%+*h)","ADC,A",
  "SUB B","SUB C","SUB D","SUB E","SUB I%h","SUB I%l","SUB (I%+*h)","SUB A",
  "SBC B","SBC C","SBC D","SBC E","SBC I%h","SBC I%l","SBC (I%+*h)","SBC A",
  "AND B","AND C","AND D","AND E","AND I%h","AND I%l","AND (I%+*h)","AND A",
  "XOR B","XOR C","XOR D","XOR E","XOR I%h","XOR I%l","XOR (I%+*h)","XOR A",
  "OR B","OR C","OR D","OR E","OR I%h","OR I%l","OR (I%+*h)","OR A",
  "CP B","CP C","CP D","CP E","CP I%h","CP I%l","CP (I%+*h)","CP A",
  "RET NZ","POP BC","JP NZ,#h","JP #h","CALL NZ,#h","PUSH BC","ADD *h","RST 00h",
  "RET Z","RET","JP Z,#h","PFX_CB","CALL Z,#h","CALL #h","ADC *h","RST 08h",
  "RET NC","POP DE","JP NC,#h","OUTA (*h)","CALL NC,#h","PUSH DE","SUB *h","RST 10h",
  "RET C","EXX","JP C,#h","INA (*h)","CALL C,#h","PFX_DD","SBC *h","RST 18h",
  "RET PO","POP I%","JP PO,#h","EX I%,(SP)","CALL PO,#h","PUSH I%","AND *h","RST 20h",
  "RET PE","LD PC,I%","JP PE,#h","EX DE,I%","CALL PE,#h","PFX_ED","XOR *h","RST 28h",
  "RET P","POP AF","JP P,#h","DI","CALL P,#h","PUSH AF","OR *h","RST 30h",
  "RET M","LD SP,I%","JP M,#h","EI","CALL M,#h","PFX_FD","CP *h","RST 38h"
};


int z80dis(unsigned char *buf, int *Counter, char str[128])
{
  char S[80],T[80],U[80],*P,*R;
  int I, J;
    
  if((I=buf[*Counter])<0) return(0);

  memset(S,0,80);
  memset(T,0,80);
  memset(U,0,80);
  memset(str,0,128);

  sprintf(str, "%.4X: %.2X",(*Counter)++,I);

  switch(I)
  {
    case 0xCB: if((I=buf[*Counter])<0) return(0);
               sprintf(U, "%.2X",I);
               strcpy(S,MnemonicsCB[I]);
               (*Counter)++;break;
    case 0xED: if((I=buf[*Counter])<0) return(0);
               sprintf(U, "%.2X",I);
               strcpy(S,MnemonicsED[I]);
               (*Counter)++;break;
    case 0xFD: if((I=buf[*Counter])<0) return(0);
               sprintf(U, "%.2X",I);
			   if(I==0xCB){
                 (*Counter)++;
                 if((I=buf[*Counter])<0) return(0);
                 (*Counter)++;
                 if((J=buf[*Counter])<0) return(0);
                 sprintf(U, "%s%.2X%.2X",U,I,J);
                 sprintf(S,"%s, (IY+%.2X)", MnemonicsCB[J], I);
			   }else{
                 strcpy(S,MnemonicsXX[I]);
                 if(P=strchr(S,'%')) *P='Y';}
               (*Counter)++;break;
    case 0xDD: if((I=buf[*Counter])<0) return(0);
               sprintf(U, "%.2X",I);
			   if(I==0xCB){
                 (*Counter)++;
                 if((I=buf[*Counter])<0) return(0);
                 (*Counter)++;
                 if((J=buf[*Counter])<0) return(0);
                 sprintf(U, "%s%.2X%.2X",U,I,J);
                 sprintf(S,"%s, (IX+%.2X)", MnemonicsCB[J], I);
			   }else{
                 strcpy(S,MnemonicsXX[I]);
                 if(P=strchr(S,'%')) *P='X';}
               (*Counter)++;break;
    default:   strcpy(S,Mnemonics[I]);
  }

  if(P=strchr(S,'*'))
  {
    if((I=buf[*Counter])<0) return(0);
    sprintf(U, "%s%.2X",U, I);
    *P++='\0';(*Counter)++;
    sprintf(T,"%s%hX",S,I);
    if(R=strchr(P,'*'))
    {
      if((I=buf[*Counter])<0) return(0);
      sprintf(U, "%s%.2X",U, I);
      *R++='\0';(*Counter)++;
      sprintf(strchr(T,'\0'),"%s%hX%s",P,I,R);
    }
    else strcat(T,P);  
  }
  else if(P=strchr(S,'#'))
  {
    if((I=buf[*Counter])<0) return(0);
    (*Counter)++;
    if((J=buf[*Counter])<0) return(0);
    sprintf(U, "%s%.2X%.2X",U, I, J);
    *P++='\0';
    (*Counter)++;
    sprintf(T,"%s%hX%s",S,256*J+I,P);
  }
  else strcpy(T,S);

  strcat(str, U);
  while(strlen(str) < 18) strcat(str, " ");
  strcat(str, T);
  strcat(str, "\n");

  return 1;
}

int __fastcall z80log(unsigned int PC)
{
	static FILE *F = NULL;
	unsigned int nPC = PC;
	char ch[128];

	if (F == NULL) F = fopen("z80.txt", "wb");

	z80dis(Ram_Z80, &nPC, ch);
	fwrite(ch, 1, strlen(ch), F);

	return 0;
}

#endif // GENS_DEBUG
