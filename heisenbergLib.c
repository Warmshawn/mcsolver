#include "Python.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct Vec
{
    int dimension;
    float x,y,z;
}Vec;
// vec1=abs(vec1)
void normalize(Vec *vec1){
    float len=sqrt(vec1->x*vec1->x+vec1->y*vec1->y+vec1->z*vec1->z);
    if (len<1e-5) return;
    vec1->x/=len;
    vec1->y/=len;
    vec1->z/=len;
}
// vec1=vec2
void equal(Vec *vec1, Vec vec2){
    vec1->x=vec2.x;
    vec1->y=vec2.y;
    vec1->z=vec2.z;
}
// vec1*=c
void cTimes(Vec *vec1, float c){
    vec1->x*=c;
    vec1->y*=c;
    vec1->z*=c;
}
// vec1+=vec2
void plusEqual(Vec *vec1, Vec vec2){
    vec1->x+=vec2.x;
    vec1->y+=vec2.y;
    vec1->z+=vec2.z;
}
// vec1-=vec2
void minusEqual(Vec *vec1, Vec vec2){
    vec1->x-=vec2.x;
    vec1->y-=vec2.y;
    vec1->z-=vec2.z;
}
// vec1 dot vec2
float dot(Vec vec1, Vec vec2){
    return vec1.x*vec2.x+vec1.y*vec2.y+vec1.z*vec2.z;
}

float diagonalDot(Vec vec1, Vec vec2, Vec vec3){
    return vec1.x*vec2.x*vec3.x+
           vec1.y*vec2.y*vec3.y+
           vec1.z*vec2.z*vec3.z;
}
Vec *generateRandomVec(){
    Vec *direction=(Vec*)malloc(sizeof(Vec));
    direction->x=rand()/32767.0-0.5;
    direction->y=rand()/32767.0-0.5;
    direction->z=rand()/32767.0-0.5;
    float len2=dot(*direction, *direction);
    if (len2>0.5)
    {
        return generateRandomVec();
    }else
    {
        float len=sqrt(len2);
        direction->x/=len;
        direction->y/=len;
        direction->z/=len;
        return direction;
    }
}

typedef struct Orb
{
    int id;
    Vec spin;
    Vec transSpin;
    int nlink;
    Vec *linkStrength;
    int inBlock;
    struct Orb **linkedOrb;
    Vec onsiteAnisotropy;

    float d_onsiteEnergy;
    float sDotN;
    int isProjected;
    
}Orb;

//establishLattice(lattice, totOrbs, initSpin, maxNLinking, nlink, linkStrength);
void establishLattice(Orb *lattice, int totOrbs, float initSpin[totOrbs], float initD[totOrbs][3], float flunc, int maxNLinking, int nlink[totOrbs], float linkStrength[totOrbs][maxNLinking][3]){
    //printf("establishing whole lattice with %d orbs and %d linkings for each orb\n",totOrbs,maxNLinking);
    for(int i=0;i<totOrbs;i++){
        //printf("check point-1, entering setting %d",i);
        lattice[i].id=i;
        //lattice[i].spin.coor=(float*)malloc(3*sizeof(float));  // allocate spin vector for each orb
        lattice[i].spin.x=initSpin[i];
        lattice[i].spin.y=0;
        lattice[i].spin.z=0;

        Vec *fluncSpin=generateRandomVec();
        cTimes(fluncSpin, flunc);
        plusEqual(&lattice[i].spin,*fluncSpin);
        normalize(&lattice[i].spin);
        cTimes(&lattice[i].spin,fabs(initSpin[i]));
        free(fluncSpin);
        //printf("%.3f %.3f \n",lattice[i].spin.x, fabs(initSpin[i]));
        lattice[i].onsiteAnisotropy.x=initD[i][0];
        lattice[i].onsiteAnisotropy.y=initD[i][1];
        lattice[i].onsiteAnisotropy.z=initD[i][2];
        //printf("orb %d spin: %.3f %.3f %.3f\n",i,lattice[i].spin.coor[0],lattice[i].spin.coor[1],lattice[i].spin.coor[2]);
        lattice[i].transSpin.x=0;  // allocate trans spin vector for each orb
        lattice[i].transSpin.y=0;
        lattice[i].transSpin.z=0;
        lattice[i].nlink=nlink[i];
        //printf("check point 1, orb: %d\n",lattice[i].id);
        lattice[i].linkStrength=(Vec*)malloc(nlink[i]*sizeof(Vec)); // allocate strength for each linking
        //printf("check point 2, total links:%d\n",nlink[i]);
        for(int j=0;j<nlink[i];j++){
            //lattice[i].linkStrength[j].coor=(float*)malloc(3*sizeof(float));
            //printf("check point 3\n");
            lattice[i].linkStrength[j].x=linkStrength[i][j][0];
            lattice[i].linkStrength[j].y=linkStrength[i][j][1];
            lattice[i].linkStrength[j].z=linkStrength[i][j][2];
            //printf("check point 4, link:%d, strength: %.3f %.3f %.3f\n",j,lattice[i].linkStrength[j].coor[0],lattice[i].linkStrength[j].coor[1],lattice[i].linkStrength[j].coor[2]);
        }
    }
}

void establishLinking(Orb *lattice, int totOrbs, int maxNLinking, int nlink[totOrbs], int linkedOrb[totOrbs][maxNLinking]){
    for(int iorb=0;iorb<totOrbs;iorb++){
        lattice[iorb].linkedOrb=(Orb**)malloc(nlink[iorb]*sizeof(Orb*));
        for(int ilink=0;ilink<nlink[iorb];ilink++){
            lattice[iorb].linkedOrb[ilink]=lattice+linkedOrb[iorb][ilink];
        }
    }
}

float getCorrEnergy(Orb *source){
    float corr=0;
    for(int i=0;i<source->nlink;i++){
        corr+=diagonalDot(source->linkStrength[i],source->spin,source->linkedOrb[i]->spin);
    }
    return corr;
}

float getDeltaCorrEnergy(Orb *source){
    float corr=0;
    for(int i=0;i<source->nlink;i++){
        corr+=diagonalDot(source->linkStrength[i],source->transSpin,source->linkedOrb[i]->spin);
    }
    return corr;
}

float getDeltaOnsiteEnergy(Orb *source){
    float s1x=source->spin.x+source->transSpin.x;
    float s1y=source->spin.y+source->transSpin.y;
    float s1z=source->spin.z+source->transSpin.z;
    return source->onsiteAnisotropy.x*(s1x*s1x-source->spin.x*source->spin.x)+
           source->onsiteAnisotropy.y*(s1y*s1y-source->spin.y*source->spin.y)+
           source->onsiteAnisotropy.z*(s1z*s1z-source->spin.z*source->spin.z);
}

int expandBlock(int*beginIndex, int*endIndex, Orb *buffer[], int*blockLen, Orb *block[], Vec refDirection){
    //printf("  Buffer: now start and end pt is %d, %d.\n",*beginIndex, *endIndex);
    if(*beginIndex>*endIndex) return 0;

    // FIFO
    Orb *outOrb=buffer[*beginIndex];
    *beginIndex+=1; // pop out the first element
    
    //FILO
    //Orb *outOrb=buffer[*endIndex];
    //*endIndex-=1; // pop out the last element

    if(outOrb->isProjected==0){
        outOrb->sDotN=-2*dot(outOrb->spin,refDirection);
        equal(&outOrb->transSpin, refDirection);
        cTimes(&outOrb->transSpin, outOrb->sDotN);
        outOrb->isProjected==1;
    }
    //printf("center orb: %d\n", outOrb->id);
    //printf("projection along axis: %.3f\n", -s1n/2);
    //printf("variation of spin: %.3f %.3f %.3f\n",outOrb->transSpin.coor[0],outOrb->transSpin.coor[1],outOrb->transSpin.coor[2]);
    //printf("it has %d neighbor orbs\n",outOrb->nlink);
    int i;
    for(i=0;i<outOrb->nlink;i++){
        //float effectiveJ=diagonalDot(outOrb->linkStrength[i], refDirection, refDirection);
        //printf("the %d linking has strength %.3f %.3f %.3f (original) %.3f (effective)\n",i,outOrb->linkStrength[i].coor[0],outOrb->linkStrength[i].coor[1],outOrb->linkStrength[i].coor[2],effectiveJ);
        Orb *linkedOrb=outOrb->linkedOrb[i];
        //printf("consider to add orb %d\n",linkedOrb->id);
        //printf("      considering the %d orb which is linking to %d orb, it is %d in block \n", linkedOrb->id, outOrb->id, linkedOrb->inBlock);
        if(linkedOrb->inBlock==0){
            //printf("projection along axis: %.3f\n", s2n);
            //float corr=-s1n*diagonalDot(refDirection,outOrb->linkStrength[i],linkedOrb->spin); // bond strength
            //printf("      spin of orb %d is %.3f %.3f %.3f and bond strength is %.3f\n",linkedOrb->id,linkedOrb->spin.coor[0],linkedOrb->spin.coor[1],linkedOrb->spin.coor[2],corr);
            if (linkedOrb->isProjected==0)
            {
                linkedOrb->sDotN=-2*dot(linkedOrb->spin,refDirection);
                equal(&linkedOrb->transSpin, refDirection);
                cTimes(&linkedOrb->transSpin, linkedOrb->sDotN);
                linkedOrb->isProjected=1;
            }
            float corr=outOrb->sDotN*linkedOrb->sDotN*diagonalDot(refDirection,refDirection,outOrb->linkStrength[i])/2;
            
            //linkedOrb->d_onsiteEnergy=getDeltaOnsiteEnergy(linkedOrb);
            if(corr<0 && (1-exp(corr))>rand()/32767.0){
                //printf("          -->>fortunately it is added to block with possibility %f\n",(1-exp(2*corr)));
                // update block
                *blockLen+=1;
                block[*blockLen-1]=linkedOrb;
                linkedOrb->inBlock=1;  // register into block
                // update buffer
                *endIndex+=1;
                buffer[*endIndex]=linkedOrb;
            }
        }
    }
    return 1;
}

void blockUpdate(int totOrbs, Orb lattice[], float*p_energy, Vec *p_totSpin){
    //printf("one block update step is initializaing...\n");
    Orb *block[totOrbs];
    Orb *buffer[totOrbs];
    int seedID=rand()%totOrbs;
    block[0]=lattice+seedID;
    buffer[0]=lattice+seedID;
    block[0]->inBlock=1;
    
    int beginIndex=0, endIndex=0, blockLen=1, i, j;
    int *p_beginIndex=&beginIndex, *p_endIndex=&endIndex, *p_blockLen=&blockLen;

    Vec *refDirection=generateRandomVec();
    //refDirection.coor=(float*)malloc(3*sizeof(float));
    //equal(&refDirection, &block[0]->spin);
    //normalize(&refDirection);
    //printf("-------------------\n");
    //printf("the seed Orb is %d\n",block[0]->id);
    //printf("trial normal direction %.3f %.3f %.3f\n",refDirection.coor[0],refDirection.coor[1],refDirection.coor[2]);
    //float effectiveJ=diagonalDot(block[0]->linkStrength[0], refDirection, refDirection);
    //printf("the 0 linking has strength %.3f %.3f %.3f (original) %.3f (effective)\n",block[0]->linkStrength[0].coor[0],block[0]->linkStrength[1].coor[1],block[0]->linkStrength[2].coor[2],effectiveJ);
    while (expandBlock(p_beginIndex, p_endIndex, buffer, p_blockLen, block, *refDirection)==1)
    {
        // no code here
    }
    
    //printf("    Block size is %d\n",*p_blockLen);
    float tot_d_onsiteEnergy=0;
    for(i=0;i<*p_blockLen;i++){
        block[i]->isProjected=0;
        for (j = 0; j < block[i]->nlink; j++)
        {
            // exchange anisotropy
            block[i]->linkedOrb[j]->isProjected=0;
            Vec originalSj, Sj_parallel;
            equal(&originalSj, block[i]->linkedOrb[j]->spin);
            equal(&Sj_parallel,block[i]->linkedOrb[j]->transSpin);
            cTimes(&Sj_parallel,0.5);
            plusEqual(&originalSj,Sj_parallel);
            tot_d_onsiteEnergy+=block[i]->sDotN*diagonalDot(originalSj,block[i]->linkStrength[j],*refDirection);
        }
        // single-ion anisotropy
        tot_d_onsiteEnergy+=getDeltaOnsiteEnergy(block[i]);
        
    }
    for(i=0;i<*p_blockLen;i++) block[i]->inBlock=0;
    free(refDirection);
    // process the onsite anisotropy
    if(tot_d_onsiteEnergy<=0 || exp(-tot_d_onsiteEnergy)>rand()/32767.0){
        for(i=0;i<*p_blockLen;i++){
            plusEqual(&block[i]->spin, block[i]->transSpin);
            //printf("    after update orb %d spin converted to %.3f %.3f %.3f\n",block[i]->id,block[i]->spin.coor[0],block[i]->spin.coor[1],block[i]->spin.coor[2]);
            //block[i]->inBlock=0;
            plusEqual(p_totSpin,block[i]->transSpin);
        }
        *p_energy=0.;
        for(i=0;i<totOrbs;i++){
            *p_energy+=getCorrEnergy(lattice+i);
        }
        *p_energy/=2;
    }
}

void localUpdate(int totOrbs, Orb lattice[], float *p_energy, Vec *p_totSpin){
    //printf("start local updating\n");
    int seedID=rand()%totOrbs;  // chose one orb
    //printf("considering %d orb, its spin is %.3f %.3f %.3f\n",
    //         seedID,lattice[seedID].spin.coor[0],lattice[seedID].spin.coor[1],lattice[seedID].spin.coor[2]);
    Vec *refDirection=generateRandomVec(); // chose new direction
    //printf("try new spin direction, ref: %.3f %.3f %.3f\n",
    //       refDirection.coor[0],refDirection.coor[1],refDirection.coor[2]);
    float s1n=-2*dot(lattice[seedID].spin,*refDirection);
    //printf("projection s1n: %.3f\n",s1n);
    equal(&lattice[seedID].transSpin,*refDirection);
    cTimes(&lattice[seedID].transSpin,s1n);
    float corr=getDeltaCorrEnergy(lattice+seedID);
    corr+=getDeltaOnsiteEnergy(lattice+seedID);
    
    //printf("lead to the translation spin vector: %.3f %.3f %.3f and delta Ecorr: %.3f\n",
    //      lattice[seedID].transSpin.coor[0],lattice[seedID].transSpin.coor[1],lattice[seedID].transSpin.coor[2],corr);
    
    if(corr<=0 || exp(-corr)>rand()/32767.0){  // new direction is energertically favoured thus accept directly
        plusEqual(p_totSpin,lattice[seedID].transSpin);
        plusEqual(&lattice[seedID].spin,lattice[seedID].transSpin);
        *p_energy+=corr;
        //printf("since new direction is energertically lowerd thus we accept, energy: %.3f\n",*p_energy);
    }
    free(refDirection);
    return;
}

PyObject * blockUpdateMC(int totOrbs, float initSpin[totOrbs], float initD[totOrbs][3], int nthermal, int nsweep, 
                   int maxNLinking, int nlink[totOrbs], float linkStrength[totOrbs][maxNLinking][3], int linkedOrb[totOrbs][maxNLinking],
                   float flunc, float h){
    // initialize lattice
    Orb lattice[totOrbs];
    //printf("hello here is C lib\n");
    establishLattice(lattice, totOrbs, initSpin, initD, flunc, maxNLinking, nlink, linkStrength);
    establishLinking(lattice, totOrbs, maxNLinking, nlink, linkedOrb);

    // initialize measurement
    float energy=0;
    float *p_energy=&energy;
    Vec totSpin;
    totSpin.x=0;totSpin.y=0;totSpin.z=0;
    Vec*p_totSpin=&totSpin;
    for(int i=0;i<totOrbs;i++) plusEqual(p_totSpin, lattice[i].spin);
    
    // initialize block
    for(int i=0;i<totOrbs;i++) {lattice[i].inBlock=0;lattice[i].isProjected=0;};

    for(int i=0;i<nthermal;i++) blockUpdate(totOrbs, lattice, p_energy, p_totSpin); //thermalization

    // printf("start sweeping\n");
    PyObject *xspinData, *yspinData, *zspinData, *energyData;
    //float len;
    xspinData=PyTuple_New(nsweep);
    yspinData=PyTuple_New(nsweep);
    zspinData=PyTuple_New(nsweep);
    energyData=PyTuple_New(nsweep);
    for(int i=0;i<nsweep;i++){
        blockUpdate(totOrbs, lattice, p_energy, p_totSpin);
        PyTuple_SetItem(xspinData, i, PyFloat_FromDouble(p_totSpin->x));
        PyTuple_SetItem(yspinData, i, PyFloat_FromDouble(p_totSpin->y));
        PyTuple_SetItem(zspinData, i, PyFloat_FromDouble(p_totSpin->z));
        PyTuple_SetItem(energyData, i, PyFloat_FromDouble(*p_energy));
    }
    PyObject *Data;
    Data=PyTuple_New(4);
    PyTuple_SetItem(Data, 0, xspinData);
    PyTuple_SetItem(Data, 1, yspinData);
    PyTuple_SetItem(Data, 2, zspinData);
    PyTuple_SetItem(Data, 3, energyData);
    return Data;
}


// self.totOrbs, initSpin, nthermal, nsweep, maxNLinking, nlinking, linkStrength, linkData
PyObject * localUpdateMC(int totOrbs, float initSpin[totOrbs], float initD[totOrbs][3], int nthermal, int nsweep, 
                   int maxNLinking, int nlink[totOrbs], float linkStrength[totOrbs][maxNLinking][3], int linkedOrb[totOrbs][maxNLinking],
                   float flunc, float h){
    // initialize lattice
    Orb lattice[totOrbs];
    //printf("hello here is C lib\n");
    establishLattice(lattice, totOrbs, initSpin, initD, flunc, maxNLinking, nlink, linkStrength);
    establishLinking(lattice, totOrbs, maxNLinking, nlink, linkedOrb);

    // initialize measurement
    float energy=0;
    float *p_energy=&energy;
    for(int i=0;i<totOrbs;i++){
        *p_energy+=getCorrEnergy(lattice+i);
    }
    *p_energy/=2;
    Vec totSpin;
    totSpin.x=0;totSpin.y=0;totSpin.z=0;
    Vec*p_totSpin=&totSpin;
    for(int i=0;i<totOrbs;i++) {
        //printf("%.3f %.3f %.3f\n",lattice[i].spin.x,lattice[i].spin.y,lattice[i].spin.y);
        plusEqual(&totSpin, lattice[i].spin);
    }
    
    //localUpdate(totOrbs, lattice, p_energy, p_totSpin);
    //printf("initial total spin: %.3f %.3f %.3f energy: %.3f\n",totSpin.x,totSpin.y,totSpin.z,*p_energy);
    for(int i=0;i<totOrbs*nthermal;i++) localUpdate(totOrbs, lattice, p_energy, p_totSpin); //thermalization

    PyObject *xspinData, *yspinData, *zspinData, *energyData;
    //float len;
    xspinData=PyTuple_New(nsweep);
    yspinData=PyTuple_New(nsweep);
    zspinData=PyTuple_New(nsweep);
    energyData=PyTuple_New(nsweep);
    for(int i=0;i<nsweep;i++){
        float energyAvg=0.0;
        Vec spinAvg;
        spinAvg.x=0;spinAvg.y=0;spinAvg.z=0;
        for(int j=0;j<totOrbs;j++){
            localUpdate(totOrbs, lattice, p_energy, p_totSpin);
            energyAvg+=*p_energy;
            plusEqual(&spinAvg, *p_totSpin);
        }
        //printf("after local updates, total spin is: %.3f %.3f %.3f\n",p_totSpin->coor[0],p_totSpin->coor[1],p_totSpin->coor[2]);
        //len=sqrt(dot(*p_totSpin, *p_totSpin));
        PyTuple_SetItem(xspinData, i, PyFloat_FromDouble(spinAvg.x/totOrbs));
        PyTuple_SetItem(yspinData, i, PyFloat_FromDouble(spinAvg.y/totOrbs));
        PyTuple_SetItem(zspinData, i, PyFloat_FromDouble(spinAvg.z/totOrbs));
        PyTuple_SetItem(energyData, i, PyFloat_FromDouble(energyAvg/totOrbs));
    }
    
    PyObject *Data;
    Data=PyTuple_New(4);
    PyTuple_SetItem(Data, 0, xspinData);
    PyTuple_SetItem(Data, 1, yspinData);
    PyTuple_SetItem(Data, 2, zspinData);
    PyTuple_SetItem(Data, 3, energyData);
    return Data;
}