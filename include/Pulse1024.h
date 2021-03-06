class Pulse1024{
	private:
		unsigned char analyze_flag;
		float waveform_cfd[1024];
		int minidx;

		//TF1* f1;
		//TF1* f1 = new TF1("f1", "pol3", -1, 2);

		void waveformcopyf(Float_t arrdest[1024],const Float_t arrsour[1024]){
			for (int iwft=0; iwft<1024; iwft++){
				arrdest[iwft]=arrsour[iwft];
			}
		}

		int ped_flag_check();
		int cfd_flag_check();
		int min_flag_check();

	public:
		UInt_t EventSize, Group, StartIndexCell, Channel, EventNumber, TrigTimeTag;
		//UInt_t EventSize, BoardID, StartIndexCell, Channel, EventNumber, TrigTimeTag;
		float waveform[1024];
		float pedestal, rPH, QDC, TDC;

		void pedestalf(int startidx=0, int endidx=200);
		void rPHf();
		void QDCf(int startidx=200, int endidx=950);
		void CFDf(int delay, float fraction);
		void TDCf(float sampling_rate, int startidx=200, int endidx=400);
		void fitTDCf(float sampling_rate, int startidx=200, int endidx=400);
		void Analyze(int ps=0, int pe=200, int qs=200, int qe=950, int cd=0, float cf=1, float tr=1, int ts=200, int te=400);
		void Waveform_Ext(float arrdest[1024]);
		float FastTrig_Analyze(float sampling_rate, float voltage_l=1024, float voltage_t=2048 );

		Pulse1024(UInt_t _header[6]){
		//Pulse1024(UInt_t _header[6], float _waveform[1024]){
			EventSize=_header[0];
			Group=_header[1];
			//BoardID=_header[1];
			StartIndexCell=_header[2];
			Channel=_header[3];
			EventNumber=_header[4];
			TrigTimeTag=_header[5];
			//waveformcopyf(waveform,_waveform);
			analyze_flag = 0x00;
			//printf("Pulse1024 init, eventnumber %u\n", EventNumber);
		}
		~Pulse1024(){
			//delete f1;
		}
};

inline int Pulse1024::ped_flag_check(){
	if(analyze_flag & 0x01) return 1;
	printf("\npedestalf first\n",stderr);
	return 0;
}

inline int Pulse1024::cfd_flag_check(){
	ped_flag_check();
	if(analyze_flag & 0x02) return 1;
	printf("\nCFDf first\n",stderr);
	return 0;
}
inline int Pulse1024::min_flag_check(){
	if(analyze_flag & 0x04) return 1;
	printf("\nrPHf first\n",stderr);
	return 0;
}

void Pulse1024::Analyze(int ps=0, int pe=200, int qs=200, int qe=950, int cd=0, float cf=1, float tr=1, int ts=200, int te=400){
	pedestalf(ps,pe);
	rPHf();
	QDCf(qs,qe);
	CFDf(cd,cf);
	TDCf(tr,ts,te);
	//fitTDCf(tr,ts,te);
	return;
}

void Pulse1024::pedestalf(int startidx=0, int endidx=200){
	float pedsum=0;
	//printf("EventNumber %u &pedsum %x \n", EventNumber, &pedsum);
	for (int iwft=startidx; iwft<endidx; iwft++){
		pedsum += waveform[iwft];
	}
	pedestal= (pedsum/(float)(endidx-startidx));
	analyze_flag |= 0x01;
	return;
}

void Pulse1024::rPHf(){
	if(!ped_flag_check()) return;
	float ADCmin=4098;
	int minidx_=0;
	//printf("EventNumber %u &ADCmin %x \n", EventNumber, &ADCmin);
	for (int i=0; i<950; i++){
		if (ADCmin>waveform[i]){
			ADCmin = waveform[i];
			minidx_= i;
		}
	}
	//rPH = pedestal;
	//rPH = 4096-ADCmin;
	rPH = pedestal-ADCmin;
	minidx = minidx_;
	//printf("\nminidx = %d\n", minidx);
	analyze_flag |= 0x04;
	return;
}

void Pulse1024::QDCf(int startidx=200, int endidx=950){
	if(!ped_flag_check()) return;
	float QDCsum=0;
	//printf("EventNumber %u &QDCsum %x \n", EventNumber, &QDCsum);
	for (int iwft=startidx; iwft<endidx; iwft++){
		QDCsum += waveform[iwft];
	}
	QDC = (endidx-startidx)*pedestal-QDCsum;
	return;
}

void Pulse1024::CFDf(Int_t delay, float fraction){
	if(!ped_flag_check()) return;
	for (int i=0; i<1024-delay; i++){
		waveform_cfd[i]= -fraction*(pedestal-waveform[i+delay])+(pedestal-waveform[i]);
	}
	for (int i=1024-delay; i<1024; i++){
		waveform_cfd[i]= -1;
	}
	analyze_flag |= 0x02;
	return;
}

void Pulse1024::TDCf(float sampling_rate, int startidx=200, int endidx=400){
	if(!cfd_flag_check()) return;
	if(!min_flag_check()) return;
	//TDC=(float)minidx/sampling_rate;
	//return;
	int minwft=0;
	int maxwft=0;
	float iwfttorecord=-1;
	float timetorecord=-2;
	for (int iwft=minidx-100; iwft<minidx+100; iwft++){
		if(iwft<0) continue;
		if(iwft>950) continue;
	//for (int iwft=startidx; iwft<endidx; iwft++){
		if(waveform_cfd[iwft]>waveform_cfd[maxwft]){maxwft=iwft;}
		if(waveform_cfd[iwft]<waveform_cfd[minwft]){minwft=iwft;}
	}
	if(minwft>maxwft){TDC=-4; return -4;}
		//printf("\nminwft(%d)>maxwft(%d)\n",minwft,maxwft);
	for (int iwft=minwft; iwft<maxwft+1; iwft++){
		if(iwft==maxwft){
			//printf("\nerr -3 rPH %.2f QDC %.2f\n",rPH,QDC);
			//printf("\nfail to find the pattern btw waveform_cfd[%d]=%.2f,waveform_cfd[%d]=%.2f\n", minwft, waveform_cfd[minwft], maxwft, waveform_cfd[maxwft]);
			TDC=-3; return -3; 
		}
		if(waveform_cfd[iwft+0]<=0 && 0<=waveform_cfd[iwft+1] && waveform_cfd[iwft+1]<=waveform_cfd[iwft+2]){
			iwfttorecord = (float)iwft-(waveform_cfd[iwft+0])/(waveform_cfd[iwft+1]-waveform_cfd[iwft+0]);
			timetorecord = (float)iwfttorecord/sampling_rate;	//0.4ns per sample
			break;
		}
	}
	TDC=timetorecord;
	return;
}

void Pulse1024::fitTDCf(float sampling_rate, int startidx=200, int endidx=400){
	if(!cfd_flag_check()) return;
	Int_t minwft=0;
	Int_t maxwft=0;
	Double_t iwfttorecord=-1;
	Double_t timetorecord=-2;
	for (int iwft=minidx-100; iwft<minidx+100; iwft++){
		if(iwft<0) continue;
		if(iwft>950) continue;
	//for (int iwft=startidx; iwft<endidx; iwft++){
		if(waveform_cfd[iwft]>waveform_cfd[maxwft]){maxwft=iwft;}
		if(waveform_cfd[iwft]<waveform_cfd[minwft]){minwft=iwft;}
	}
	if(minwft>maxwft) return -4;
	int iwft=0;
	for (iwft=minwft; iwft<maxwft+1; iwft++){
		if(iwft==maxwft) return -3; 
		if(waveform_cfd[iwft-1]<=waveform_cfd[iwft+0] && waveform_cfd[iwft+0]<=0 && 0<=waveform_cfd[iwft+1] && waveform_cfd[iwft+1]<=waveform_cfd[iwft+2]){
			break;
		}
	}

	double a[4];
	a[0]=	( 0*waveform_cfd[iwft-1]+6*waveform_cfd[iwft+0]+0*waveform_cfd[iwft+1]+0*waveform_cfd[iwft+2])/6.0;
	a[1]=	(-2*waveform_cfd[iwft-1]-3*waveform_cfd[iwft+0]+6*waveform_cfd[iwft+1]-1*waveform_cfd[iwft+2])/6.0;
	a[2]=	( 3*waveform_cfd[iwft-1]-6*waveform_cfd[iwft+0]+3*waveform_cfd[iwft+1]+0*waveform_cfd[iwft+2])/6.0;
	a[3]=	(-1*waveform_cfd[iwft-1]+3*waveform_cfd[iwft+0]-3*waveform_cfd[iwft+1]+1*waveform_cfd[iwft+2])/6.0;
	if( 0<a[3] && 0<a[2] && a[2]<3.0*a[3] && a[1]*9.0*a[3]*a[3]<a[2]*a[2]){
		printf("\nnegative slope at threshold crossing\n");
		return -5;
	}

	TF1* f1 = new TF1("f1", "pol3", -1, 2);
	f1->SetParameter(0, a[0]);
	f1->SetParameter(1, a[1]);
	f1->SetParameter(2, a[2]);
	f1->SetParameter(3, a[3]);
	iwfttorecord = iwft+(f1->GetX(0, 0, 1));
	f1->~TF1();	


	//Double_t x_wf[4] = {waveform_cfd[iwft],waveform_cfd[iwft+1],waveform_cfd[iwft+2],waveform_cfd[iwft+3]};
	//Double_t y_wft[4] = {(double)iwft,(double)iwft+1,(double)iwft+2,(double)iwft+3};
	//TGraph *gr = new TGraph(4, x_wf,y_wft);
	//TF1 *f1 = new TF1("f1", "pol3", waveform_cfd[iwft], waveform_cfd[iwft+3]+0.0001);
	//gr->Fit(f1, "Q","",waveform_cfd[iwft],waveform_cfd[iwft+3]);	// for the best? 4 by 4 linear equation can be solved... make some practice!
	//iwfttorecord = f1->Eval(0,0,0,0);

	timetorecord = (float)iwfttorecord/sampling_rate;	//0.4ns per sample
	TDC=timetorecord;
	return;

/*	double a[4];
	a[0] = ( 0*waveform_cfd[iwft]+6*waveform_cfd[iwft+1]+0*waveform_cfd[iwft+2]+0*waveform_cfd[iwft+3])/6.0;
	a[1] = (-2*waveform_cfd[iwft]-3*waveform_cfd[iwft+1]+6*waveform_cfd[iwft+2]-1*waveform_cfd[iwft+3])/6.0;
	a[2] = ( 3*waveform_cfd[iwft]-6*waveform_cfd[iwft+1]+3*waveform_cfd[iwft+2]+0*waveform_cfd[iwft+3])/6.0;
	a[3] = (-1*waveform_cfd[iwft]+3*waveform_cfd[iwft+1]-3*waveform_cfd[iwft+2]+1*waveform_cfd[iwft+3])/6.0;
	double A[4];
	A[3]=1.0;	A[2]=a[2]/a[3]; A[1]=a[1]/a[3]; A[0]=a[0]/a[3]; 

	double b[4];
	b[3]=1.0;	b[2]=0;	b[1]=A[1]-A[2]*A[2]/3.0;	b[0]=A[0]-A[1]*A[2]/3.0+A[2]*A[2]*A[2]*2.0/27.0;
	double p=b[1]; double q=b[0];

	double D=4.0*p*p*p+27.0*q*q;

	if(D<0) TDC=200.0;
	if(D>=0) TDC=250.0;
	return;
*/


}

float Pulse1024::FastTrig_Analyze(float sampling_rate, float voltage_l=1024, float voltage_t=2048 ){
	int startidx=0;
	float irecord;

	int iwft=0;
	int low_flag=1;
	for(iwft=70; iwft<950; iwft++){
		low_flag=1;
		for(int j=0; j<15; j++){
			if(waveform[iwft+j]>voltage_l){ low_flag=0; break;}
		}
		if(!low_flag){
			startidx=iwft;
			break;
		}
	}
	for(iwft=startidx+15; iwft<950; iwft++){
		if(iwft==950) return -4;
		if(waveform[iwft-1]<waveform[iwft+0] && waveform[iwft+0]<voltage_t && voltage_t<waveform[iwft+1] && waveform[iwft+1]<waveform[iwft+2]){
			break;
		}
	} 
	irecord = (float)iwft +(voltage_t-waveform[iwft])/(waveform[iwft+1]-waveform[iwft+0]);
	/*double a[4];
	a[0]=	( 0*waveform[iwft-1]+6*waveform[iwft+0]+0*waveform[iwft+1]+0*waveform[iwft+2])/6.0;
	a[1]=	(-2*waveform[iwft-1]-3*waveform[iwft+0]+6*waveform[iwft+1]-1*waveform[iwft+2])/6.0;
	a[2]=	( 3*waveform[iwft-1]-6*waveform[iwft+0]+3*waveform[iwft+1]+0*waveform[iwft+2])/6.0;
	a[3]=	(-1*waveform[iwft-1]+3*waveform[iwft+0]-3*waveform[iwft+1]+1*waveform[iwft+2])/6.0;
	if( 0<a[3] && 0<a[2] && a[2]<3.0*a[3] && a[1]*9.0*a[3]*a[3]<a[2]*a[2]){
		printf("\nnegative slope at threshold crossing\n");
		return -5;
	}

	TF1* f1 = new TF1("f1", "pol3", -1, 2);
	f1->SetParameter(0, a[0]);
	f1->SetParameter(1, a[1]);
	f1->SetParameter(2, a[2]);
	f1->SetParameter(3, a[3]);
	irecord = iwft+(f1->GetX(voltage_t, 0, 1));
	f1->~TF1();*/

	float timerecord = irecord/sampling_rate;	//0.4ns per sample
	return timerecord;

}



void Pulse1024::Waveform_Ext(float arrdest[1024]){
	waveformcopyf(arrdest,waveform);
	return;
}
