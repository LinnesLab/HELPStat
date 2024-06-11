/*
    FILENAME: HELPStat.cpp
    AUTHOR: Kevin Alessandro Bautista
    EMAIL: kbautis@purdue.edu

    DISCLAIMER: 
    Linnes Lab code, firmware, and software is released under the MIT License
    (http://opensource.org/licenses/MIT).
    
    The MIT License (MIT)
    
    Copyright (c) 2024 Linnes Lab, Purdue University, West Lafayette, IN, USA
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#include <HELPStat.h>

HELPStat::HELPStat() {}

AD5940Err HELPStat::AD5940Start(void) {
    uint32_t err = AD5940_MCUResourceInit();
    if(err != 0) return AD5940ERR_ERROR;
    
    /* PIN DISPLAY */
    delay(5000);
    Serial.println("Start sequence successful.");
    Serial.print("SCK: ");
    Serial.println(SCK);
    Serial.print("MOSI: ");
    Serial.println(MOSI);
    Serial.print("MISO: ");
    Serial.println(MISO);
    Serial.print("RESET: ");
    Serial.println(RESET);
    Serial.print("CS: ");
    Serial.println(CS);
    Serial.print("INTERRUPT PIN: ");
    Serial.println(ESP32_INTERRUPT);

}

int32_t HELPStat::AD5940PlatformCfg(void) {
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  AGPIOCfg_Type gpio_cfg;

  /* Use hardware reset */
  AD5940_HWReset();
  AD5940_Initialize();
  /* Platform configuration */
  /* Step1. Configure clock */
  clk_cfg.ADCClkDiv = ADCCLKDIV_1;
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC;
  clk_cfg.SysClkDiv = SYSCLKDIV_1;
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC;
  clk_cfg.HfOSC32MHzMode = bFALSE;
  clk_cfg.HFOSCEn = bTRUE;
  clk_cfg.HFXTALEn = bFALSE;
  clk_cfg.LFOSCEn = bTRUE;
  AD5940_CLKCfg(&clk_cfg);
  /* Step2. Configure FIFO and Sequencer*/
  fifo_cfg.FIFOEn = bFALSE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;                       /* 4kB for FIFO, The reset 2kB for sequencer */
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  fifo_cfg.FIFOThresh = 4;//AppIMPCfg.FifoThresh;        /* DFT result. One pair for RCAL, another for Rz. One DFT result have real part and imaginary part */
  AD5940_FIFOCfg(&fifo_cfg);
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);
  
  /* Step3. Interrupt controller */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);   /* Enable all interrupt in INTC1, so we can check INTC flags */
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE); 
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

  /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP0_INT|GP1_SLEEP|GP2_SYNC;
  gpio_cfg.InputEnSet = 0;
  gpio_cfg.OutputEnSet = AGPIO_Pin0|AGPIO_Pin1|AGPIO_Pin2;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;
  AD5940_AGPIOCfg(&gpio_cfg);
  AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);  /* Allow AFE to enter sleep mode. */
  return 0;
}

void HELPStat::AD5940ImpedanceStructInit(float startFreq, float endFreq, uint32_t numPoints) {
    AppIMPCfg_Type *pImpedanceCfg;

    AppIMPGetCfg(&pImpedanceCfg);
    /* Step1: configure initialization sequence Info */
    pImpedanceCfg->SeqStartAddr = 0;
    pImpedanceCfg->MaxSeqLen = 512; /* @todo add checker in function */

    pImpedanceCfg->RcalVal = 10000;
    pImpedanceCfg->SinFreq = 60000.0;
    pImpedanceCfg->FifoThresh = 4;
        
    /* Set switch matrix to onboard(EVAL-AD5940ELECZ) dummy sensor. */
    /* Note the RCAL0 resistor is 10kOhm. */
    pImpedanceCfg->DswitchSel = SWD_CE0;
    pImpedanceCfg->PswitchSel = SWP_RE0;
    pImpedanceCfg->NswitchSel = SWN_SE0;
    pImpedanceCfg->TswitchSel = SWT_SE0LOAD;
    
    /* The dummy sensor is as low as 5kOhm. We need to make sure RTIA is small enough that HSTIA won't be saturated. */
    pImpedanceCfg->HstiaRtiaSel = HSTIARTIA_5K;	
    
    /* Configure the sweep function. */
    pImpedanceCfg->SweepCfg.SweepEn = bTRUE;
    /* Getting some quick results with 1 kHz, but set to 10 Hz to see how slow it takes to boot up */
    pImpedanceCfg->SweepCfg.SweepStart = startFreq;	/* Start from 1kHz */
    pImpedanceCfg->SweepCfg.SweepStop = endFreq;		/* Stop at 100kHz */
    // pImpedanceCfg->SweepCfg.SweepPoints = 51;		/* Points is 101 */
    
    /* Using custom points - defaults to downward log sweep */
    if(startFreq > endFreq) pImpedanceCfg->SweepCfg.SweepPoints = (uint32_t)(1.5 + (log10(startFreq) - log10(endFreq)) * (numPoints)) - 1;
    else pImpedanceCfg->SweepCfg.SweepPoints = (uint32_t)(1.5 + (log10(endFreq) - log10(startFreq)) * (numPoints)) - 1;

    /* Print Statement - REMOVE*/
    Serial.println(pImpedanceCfg->SweepCfg.SweepPoints);

    pImpedanceCfg->SweepCfg.SweepLog = bTRUE;

    /* Configure Power Mode. Use HP mode if frequency is higher than 80kHz. */
    pImpedanceCfg->PwrMod = AFEPWR_LP;

    /* Configure filters if necessary */
    pImpedanceCfg->ADCSinc3Osr = ADCSINC3OSR_2;		/* Sample rate is 800kSPS/2 = 400kSPS */
    pImpedanceCfg->DftNum = DFTNUM_16384;
    pImpedanceCfg->DftSrc = DFTSRC_SINC3;
}

int32_t HELPStat::ImpedanceShowResult(uint32_t *pData, uint32_t DataCount)
{
  float freq;

  fImpPol_Type *pImp = (fImpPol_Type*)pData;
  AppIMPCtrl(IMPCTRL_GETFREQ, &freq);

  // printf("Freq:%.2f ", freq);
  // printf("Freq, RzMag (ohm), RzPhase (degrees)\n");
  /*Process data*/
  for(int i=0;i<DataCount;i++)
  {
    // printf("RzMag: %f Ohm , RzPhase: %f \n",pImp[i].Magnitude,pImp[i].Phase*180/MATH_PI);
    // printf("%.2f, %f, %f \n", freq, pImp[i].Magnitude,pImp[i].Phase*180/MATH_PI);
    printf("%0.3f, %f, %f, %f, %f, \n", freq, pImp[i].Magnitude,pImp[i].Phase, pImp[i].Magnitude * cos(pImp[i].Phase), -pImp[i].Magnitude * sin(pImp[i].Phase));

  }

  return 0;
}

void HELPStat::AD5940_Main(float startFreq, float endFreq, uint32_t numPoints, uint32_t gainArrSize, calHSTIA* gainArr)
{
  uint32_t temp;  

  _gainArrSize = gainArrSize;
  printf("Gain array size: %d\n", _gainArrSize);
  for(uint32_t i = 0; i < _gainArrSize; i++)
  {
    _gainArr[i] = gainArr[i];
    // _arrHSTIA[i] = arrHSTIA[i];
  }

  /* Can also try moving this to .h file */
  uint32_t AppBuff[APPBUFF_SIZE];

  AD5940PlatformCfg();
  AD5940ImpedanceStructInit(startFreq, endFreq, numPoints);
  
  AppIMPInit(AppBuff, APPBUFF_SIZE);    /* Initialize IMP application. Provide a buffer, which is used to store sequencer commands */
  AppIMPCtrl(IMPCTRL_START, 0);          /* Control IMP measurement to start. Second parameter has no meaning with this command. */
  printf("Freq, RzMag (ohm), RzPhase (degrees). Rreal, Rimag\n");

  while(1)
  {
    if(AD5940_GetMCUIntFlag())
    {
      AD5940_ClrMCUIntFlag();
      temp = APPBUFF_SIZE;
      AppIMPISR(AppBuff, &temp);
      ImpedanceShowResult(AppBuff, temp);
    }
  }
}

void HELPStat::DFTPolling_Main(void) {
  /* TESTING TO SEE IF THIS WORKS ON ITS OWN */
  DSPCfg_Type dsp_cfg;
  WGCfg_Type wg_cfg;

  /* Use hardware reset */
  AD5940_HWReset();
  /* Firstly call this function after reset to initialize AFE registers. */
  AD5940_Initialize();
  
  /* Configure AFE power mode and bandwidth */
  AD5940_AFEPwrBW(AFEPWR_LP, AFEBW_250KHZ);
  
  AD5940_StructInit(&dsp_cfg, sizeof(dsp_cfg));
  /* Initialize ADC basic function */
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_VCE0;
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_VSET1P1;
  dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_1;
  
  /* Initialize ADC filters ADCRawData-->SINC3-->SINC2+NOTCH */
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_4;
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = ADCSINC2OSR_1333;
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_2;         /* Don't care about it. Average function is only used for DFT */
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;        /* If ADC clock is 32MHz, then set it to ADCRATE_1P6MHZ. Default is 16MHz, use ADCRATE_800KHZ. */
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE;                 /* SINC2+Notch is one block, when bypass notch filter, we can get fresh data from SINC2 filter. */
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE;                /* We use SINC3 filter. */
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bTRUE;        /* Enable the SINC2+Notch block. You can also use function AD5940_AFECtrlS */
  dsp_cfg.DftCfg.DftNum = DFTNUM_16384;
  dsp_cfg.DftCfg.DftSrc = DFTSRC_SINC3;
  AD5940_DSPCfgS(&dsp_cfg);

  AD5940_StructInit(&wg_cfg, sizeof(wg_cfg));
  wg_cfg.WgType = WGTYPE_SIN;
  wg_cfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(1000.0, 16000000.0);  /* 10kHz */
  AD5940_WGCfgS(&wg_cfg);

  /* Enable all interrupt at Interrupt Controller 1. So we can check the interrupt flag */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE); 
  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_SINC2NOTCH|AFECTRL_WG, bTRUE);
  AD5940_AFECtrlS(AFECTRL_DFT, bTRUE);
  AD5940_ADCConvtCtrlS(bTRUE);
  
  while(1)
  {
    int32_t real, image;
    if(AD5940_INTCTestFlag(AFEINTC_1,AFEINTSRC_DFTRDY))
    {
      AD5940_INTCClrFlag(AFEINTSRC_DFTRDY);
      real = AD5940_ReadAfeResult(AFERESULT_DFTREAL);
      if(real&(1<<17))
        real |= 0xfffc0000; /* Data is 18bit in two's complement, bit17 is the sign bit */
      printf("DFT: %d,", real);      
      image = AD5940_ReadAfeResult(AFERESULT_DFTIMAGE);
      if(image&(1<<17))
        image |= 0xfffc0000; /* Data is 18bit in two's complement, bit17 is the sign bit */
      printf("%d,", image);      
      printf("Mag:%f\n", sqrt((float)real*real + (float)image*image));
    }
  }
}

/*
Sets initial configuration for HSLOOP (will change in measurement stage)
and configures CLK, AFE, ADC, GPIO/INTs, and DSP for analysis. 
Most will stay constant but some will change depending on frequency 

11/30/2023 - Need to view bias with an oscilloscope to verify 

02/12/2024 - Bias was verified. Waveforms look somewhat distorted though for some
frequencies and subject to noise, but frequency at 200 kHz and 0.1 Hz was accurate
and bias works. 
*/
void HELPStat::AD5940_TDD(float startFreq, float endFreq, uint32_t numPoints, float biasVolt, float zeroVolt, float rcalVal, calHSTIA *gainArr, int gainArrSize, int extGain, int dacGain) {

  // SETUP Cfgs
  CLKCfg_Type clk_cfg;
  AGPIOCfg_Type gpio_cfg;
  ClksCalInfo_Type clks_cal;
  LPAmpCfg_Type LpAmpCfg;
  
  // DFT / ADC / WG / HSLoop Cfgs
  AFERefCfg_Type aferef_cfg;
  HSLoopCfg_Type HsLoopCfg;
  DSPCfg_Type dsp_cfg;

  float sysClkFreq = 16000000.0; // 16 MHz
  float adcClkFreq = 16000000.0; // 16 MHz
  float sineVpp = 200.0; // 200 mV 
  _rcalVal = rcalVal;

  /* Configuring the Gain Array */
  _gainArrSize = gainArrSize;
  printf("Gain array size: %d\n", _gainArrSize);
  for(uint32_t i = 0; i < _gainArrSize; i++)
  {
    _gainArr[i] = gainArr[i];
    // _arrHSTIA[i] = arrHSTIA[i];
  }

  /* Use hardware reset */
  AD5940_HWReset();
  AD5940_Initialize();

  /* Platform configuration */
  /* Step1. Configure clock - NEED THIS */
  clk_cfg.ADCClkDiv = ADCCLKDIV_1; // Clock source divider - ADC
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC; // Enables internal high frequency 16/32 MHz clock as source
  clk_cfg.SysClkDiv = SYSCLKDIV_1; // Clock source divider - System
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC; // Enables internal high frequency 16/32 MHz clock as source 
  clk_cfg.HfOSC32MHzMode = bFALSE; // Sets it to 16 MHz
  clk_cfg.HFOSCEn = bTRUE; // Enables the internal 16 / 32 MHz source
  clk_cfg.HFXTALEn = bFALSE; // Disables any need for external clocks
  clk_cfg.LFOSCEn = bTRUE; // Enables 32 kHz clock for timing / wakeups
  AD5940_CLKCfg(&clk_cfg); // Configures the clock
  Serial.println("Clock setup successfully.");

  /* Step3. Interrupt controller */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);   /* Enable all interrupt in INTC1, so we can check INTC flags */
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT); // Clears all INT flags

  /* Set INT0 source to be DFT READY */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DFTRDY, bTRUE); 
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT); // clears all flags 
  Serial.println("INTs setup successfully.");

  /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP0_INT;

  gpio_cfg.InputEnSet = 0; // Disables any GPIOs as inputs
  gpio_cfg.OutputEnSet = AGPIO_Pin0; // Enables GPIOs as outputs

  gpio_cfg.OutVal = 0; // Value for the output 
  gpio_cfg.PullEnSet = 0; // Disables any GPIO pull-ups / Pull-downs

  AD5940_AGPIOCfg(&gpio_cfg); // Configures the GPIOs
  Serial.println("GPIOs setup successfully.");

  /* CONFIGURING FOR DFT */
  // AFE Configuration 
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);  /* Initializing to disabled state */

  // Enabling high power bandgap since we're using High Power DAC
  // Enables operation at higher frequencies 
  aferef_cfg.HpBandgapEn = bTRUE;

  aferef_cfg.Hp1V1BuffEn = bTRUE; // Enables 1v1 buffer
  aferef_cfg.Hp1V8BuffEn = bTRUE; // Enables 1v8 buffer

  /* Not going to discharge capacitors - haven't seen this ever used */
  aferef_cfg.Disc1V1Cap = bFALSE;
  aferef_cfg.Disc1V8Cap = bFALSE;

  /* Disabling buffers and current limits*/
  aferef_cfg.Hp1V8ThemBuff = bFALSE;
  aferef_cfg.Hp1V8Ilimit = bFALSE;

  /* Disabling low power buffers */
  aferef_cfg.Lp1V1BuffEn = bFALSE;
  aferef_cfg.Lp1V8BuffEn = bFALSE;

  /* LP reference control - turn off if no bias */
  if((biasVolt == 0.0f) && (zeroVolt == 0.0f))
  {
    aferef_cfg.LpBandgapEn = bFALSE;
    aferef_cfg.LpRefBufEn = bFALSE;
    printf("No bias today!\n");
  }
  else
  {
    aferef_cfg.LpBandgapEn = bTRUE;
    aferef_cfg.LpRefBufEn = bTRUE;
    printf("We have bias!\n");
  }

  /* Doesn't enable boosting buffer current */
  aferef_cfg.LpRefBoostEn = bFALSE;
  AD5940_REFCfgS(&aferef_cfg);	// Configures the AFE 
  Serial.println("AFE setup successfully.");
  
  /* Disconnect SE0 from LPTIA - double check this too */
	LpAmpCfg.LpAmpPwrMod = LPAMPPWR_NORM;
  LpAmpCfg.LpPaPwrEn = bFALSE; //bTRUE
  LpAmpCfg.LpTiaPwrEn = bFALSE; //bTRUE
  LpAmpCfg.LpTiaRf = LPTIARF_1M;
  LpAmpCfg.LpTiaRload = LPTIARLOAD_100R;
  LpAmpCfg.LpTiaRtia = LPTIARTIA_OPEN; /* Disconnect Rtia to avoid RC filter discharge */
  LpAmpCfg.LpTiaSW = LPTIASW(7)|LPTIASW(8)|LPTIASW(12)|LPTIASW(13); 
	AD5940_LPAMPCfgS(&LpAmpCfg);
  Serial.println("SE0 disconnected from LPTIA.");
  
  // Configuring High Speed Loop (high power loop)
  /* Vpp * BufGain * DacGain */
  // HsLoopCfg.HsDacCfg.ExcitBufGain = EXCITBUFGAIN_0P25;
  // HsLoopCfg.HsDacCfg.HsDacGain = HSDACGAIN_0P2;

  _extGain = extGain; 
  _dacGain = dacGain;

  HsLoopCfg.HsDacCfg.ExcitBufGain = _extGain;
  HsLoopCfg.HsDacCfg.HsDacGain = _dacGain;

  /* For low power / frequency measurements use 0x1B, o.w. 0x07 */
  HsLoopCfg.HsDacCfg.HsDacUpdateRate = 0x1B;
  HsLoopCfg.HsTiaCfg.DiodeClose = bFALSE;

  /* Assuming no bias - default to 1V1 bias */
  if((biasVolt == 0.0f) && (zeroVolt == 0.0f))
  {
    HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;
    printf("HSTIA bias set to 1.1V.\n");
  }
  else 
  {
    HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_VZERO0;
    printf("HSTIA bias set to Vzero.\n");
  }

  /* Sets feedback capacitor on HSTIA */
  HsLoopCfg.HsTiaCfg.HstiaCtia = 31; /* 31pF + 2pF */

  /* No load and RTIA on the D Switch*/
  HsLoopCfg.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  HsLoopCfg.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_OPEN;

  /* Assuming low frequency measurement */
  HsLoopCfg.HsTiaCfg.HstiaRtiaSel = HSTIARTIA_40K;

  HsLoopCfg.SWMatCfg.Dswitch = SWD_CE0;       // Connects WG to CE0
  HsLoopCfg.SWMatCfg.Pswitch = SWP_RE0;       // Connects positive input to RE0
  HsLoopCfg.SWMatCfg.Nswitch = SWN_SE0;       // Connects negative input to SE0
  HsLoopCfg.SWMatCfg.Tswitch = SWT_SE0LOAD|SWT_TRTIA;   // Connects SEO to HSTIA via SE0Load

  _currentFreq = startFreq;
  HsLoopCfg.WgCfg.WgType = WGTYPE_SIN;
  HsLoopCfg.WgCfg.GainCalEn = bTRUE;          // Gain calibration
  HsLoopCfg.WgCfg.OffsetCalEn = bTRUE;        // Offset calibration
  printf("Current Freq: %f\n", _currentFreq);
  HsLoopCfg.WgCfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(_currentFreq, sysClkFreq);
  HsLoopCfg.WgCfg.SinCfg.SinAmplitudeWord = (uint32_t)((sineVpp/800.0f)*2047 + 0.5f);
  HsLoopCfg.WgCfg.SinCfg.SinOffsetWord = 0;
  HsLoopCfg.WgCfg.SinCfg.SinPhaseWord = 0;
  AD5940_HSLoopCfgS(&HsLoopCfg);
  Serial.println("HS Loop configured successfully");
  
  /* Configuring Sweep Functionality */
  _sweepCfg.SweepEn = bTRUE; 
  _sweepCfg.SweepLog = bTRUE;
  _sweepCfg.SweepIndex = 0; 
  _sweepCfg.SweepStart = startFreq; 
  _sweepCfg.SweepStop = endFreq;

  _startFreq = startFreq; 
  _endFreq = endFreq;
  
  // Defaulting to a logarithmic sweep. Works both upwards and downwards
  if(startFreq > endFreq) _sweepCfg.SweepPoints = (uint32_t)(1.5 + (log10(startFreq) - log10(endFreq)) * (numPoints)) - 1;
  else _sweepCfg.SweepPoints = (uint32_t)(1.5 + (log10(endFreq) - log10(startFreq)) * (numPoints)) - 1;
  printf("Number of points: %d\n", _sweepCfg.SweepPoints);
  Serial.println("Sweep configured successfully.");

   /* Configuring LPDAC if necessary */
  if((biasVolt != 0.0f) || (zeroVolt != 0.0f))
  {
    LPDACCfg_Type lpdac_cfg;
    
    lpdac_cfg.LpdacSel = LPDAC0;
    lpdac_cfg.LpDacVbiasMux = LPDACVBIAS_12BIT; /* Use Vbias to tune BiasVolt. */
    lpdac_cfg.LpDacVzeroMux = LPDACVZERO_6BIT;  /* Vbias-Vzero = BiasVolt */

    // Uses 2v5 as a reference, can set to AVDD
    lpdac_cfg.LpDacRef = LPDACREF_2P5;
    lpdac_cfg.LpDacSrc = LPDACSRC_MMR;      /* Use MMR data, we use LPDAC to generate bias voltage for LPTIA - the Vzero */
    lpdac_cfg.PowerEn = bTRUE;              /* Power up LPDAC */
    /* 
      Default bias case - centered around Vzero = 1.1V
      This works decently well. Error seems to increase as you go higher in bias.
     */
    if(zeroVolt == 0.0f)
    {
      // Edge cases 
      if(biasVolt<-1100.0f) biasVolt = -1100.0f + DAC12BITVOLT_1LSB;
      if(biasVolt> 1100.0f) biasVolt = 1100.0f - DAC12BITVOLT_1LSB;
      
      /* Bit conversion from voltage */
      // Converts the bias voltage to a data bit - uses the 1100 to offset it with Vzero
      lpdac_cfg.DacData6Bit = 0x40 >> 1;            /* Set Vzero to middle scale - sets Vzero to 1.1V */
      lpdac_cfg.DacData12Bit = (uint16_t)((biasVolt + 1100.0f)/DAC12BITVOLT_1LSB);
    }
    else
    {
      /* 
        Working decently well now.
      */
      lpdac_cfg.DacData6Bit = (uint32_t)((zeroVolt-200)/DAC6BITVOLT_1LSB);
      lpdac_cfg.DacData12Bit = (int32_t)((biasVolt)/DAC12BITVOLT_1LSB) + (lpdac_cfg.DacData6Bit * 64);
      if(lpdac_cfg.DacData12Bit < lpdac_cfg.DacData6Bit * 64) lpdac_cfg.DacData12Bit--; // compensation as per datasheet 
    } 
    lpdac_cfg.DataRst = bFALSE;      /* Do not reset data register */
    // Allows for measuring of Vbias and Vzero voltages and connects them to LTIA, LPPA, and HSTIA
    lpdac_cfg.LpDacSW = LPDACSW_VBIAS2LPPA|LPDACSW_VBIAS2PIN|LPDACSW_VZERO2LPTIA|LPDACSW_VZERO2PIN|LPDACSW_VZERO2HSTIA;
    AD5940_LPDACCfgS(&lpdac_cfg);
    Serial.println("LPDAC configured successfully.");
  }

  // /* Sets the input of the ADC to the output of the HSTIA */
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_HSTIA_N;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_HSTIA_P;

  /* Programmable gain array for the ADC */
  dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_1;
  // dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_2;
  
  /* Disables digital comparator functionality */
  memset(&dsp_cfg.ADCDigCompCfg, 0, sizeof(dsp_cfg.ADCDigCompCfg));
  
  /* Is this actually being used? */
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_16; // Impedance example uses 16 
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;	/* Tell filter block clock rate of ADC*/

  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_1P6MHZ;	/* Tell filter block clock rate of ADC*/
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = ADCSINC2OSR_22; // Oversampling ratio for SINC2
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_2; // Oversampling ratio for SINC3
  /* Using Recommended OSR of 4 for SINC3 */
  // dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_4; // Oversampling ratio for SINC3

  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE; // Bypasses Notch filter
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE; // Doesn't bypass SINC3
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bTRUE; // Enables SINC2 filter

  dsp_cfg.DftCfg.DftNum = DFTNUM_16384; // Max number of DFT points
  dsp_cfg.DftCfg.DftSrc = DFTSRC_SINC3; // Sets DFT source to SINC3
  dsp_cfg.DftCfg.HanWinEn = bTRUE;  // Enables HANNING WINDOW - recommended to always be on 
  
  /* Disables STAT block */
  memset(&dsp_cfg.StatCfg, 0, sizeof(dsp_cfg.StatCfg));
  
  AD5940_DSPCfgS(&dsp_cfg); // Sets the DFT 
  Serial.println("DSP configured successfully.");

  /* Calculating Clock Cycles to wait given DFT settings */
  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = DFTSRC_SINC3; // Source of DFT
  clks_cal.DataCount = 1L<<(DFTNUM_16384+2); /* 2^(DFTNUMBER+2) */
  clks_cal.ADCSinc2Osr = ADCSINC2OSR_22;
  clks_cal.ADCSinc3Osr = ADCSINC3OSR_2;
  clks_cal.ADCAvgNum = ADCAVGNUM_16;
  clks_cal.RatioSys2AdcClk = sysClkFreq / adcClkFreq; // Same ADC / SYSTEM CLCK FREQ
  AD5940_ClksCalculate(&clks_cal, &_waitClcks);

  /* Clears any interrupts just in case */
  AD5940_ClrMCUIntFlag();
  AD5940_INTCClrFlag(AFEINTSRC_DFTRDY);

  /* Do I need to include AFECTRL_HPREFPWR? It's the only one not here. */

   // Added bias option conditionally 
  if((biasVolt == 0.0f) && (zeroVolt == 0.0f))
  {
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                  AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                  AFECTRL_SINC2NOTCH, bTRUE);
    Serial.println("No bias applied.");
  }
  else
  {
    /* 
      Also powers the DC offset buffers that's used with LPDAC (Vbias) 
      Buffers need to be powered up here but aren't turned off in measurement like 
      the rest. This is to ensure the LPDAC and bias stays on the entire time.
    */
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                  AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                  AFECTRL_SINC2NOTCH|AFECTRL_DCBUFPWR, bTRUE);
    Serial.println("Bias is applied.");
  }

  // AD5940_SleepKeyCtrlS(SLPKEY_LOCK); // Disables Sleep Mode 

  Serial.println("Everything turned on.");
  printf("Number of points to sweep: %d\n", _sweepCfg.SweepPoints);
  printf("Bias: %f, Zero: %f\n", biasVolt, zeroVolt);
}

void HELPStat::AD5940_DFTMeasure(void) {

  SWMatrixCfg_Type sw_cfg;
  impStruct eis;

  /* Real / Imaginary components */
  int32_t realRcal, imageRcal; 
  int32_t realRz, imageRz; 

  /* Magnitude / phase */
  float magRcal, phaseRcal; 
  float magRz, phaseRz;
  float calcMag, calcPhase; // phase in rads 
  // float rcalVal = 9930; // known Rcal - measured with DMM

  // Serial.print("Recommended clock cycles: ");
  // Serial.println(_waitClcks);

  AD5940_Delay10us(_waitClcks * (1/SYSCLCK));

  /* Measuring RCAL */
  sw_cfg.Dswitch = SWD_RCAL0;
  sw_cfg.Pswitch = SWP_RCAL0;
  sw_cfg.Nswitch = SWN_RCAL1;
  sw_cfg.Tswitch = SWT_RCAL1|SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
	
	AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bTRUE);

  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);  /* Enable Waveform generator */
  // delay(500); 
  settlingDelay(_currentFreq);
  
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);  /* Start ADC convert and DFT */
  settlingDelay(_currentFreq);

  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));

  /* Polling and retrieving data from the DFT */
  pollDFT(&realRcal, &imageRcal);

  // AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  // AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));

  //wait for first data ready
  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG, bFALSE);  /* Stop ADC convert and DFT */

  sw_cfg.Dswitch = SWD_CE0;
  sw_cfg.Pswitch = SWP_RE0;
  sw_cfg.Nswitch = SWN_SE0;
  sw_cfg.Tswitch = SWT_TRTIA|SWT_SE0LOAD;
  AD5940_SWMatrixCfgS(&sw_cfg);
  // Serial.println("Switched to SE0.");

  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_WG, bTRUE);  /* Enable Waveform generator */
  // delay(500);
  settlingDelay(_currentFreq);

  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);  /* Start ADC convert and DFT */
  settlingDelay(_currentFreq);
  // delay(500);

  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));

  /* Polling and retrieving data from the DFT */
  pollDFT(&realRz, &imageRz);

  // AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  // AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));

  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG|AFECTRL_ADCPWR, bFALSE);  /* Stop ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bFALSE);

  // Serial.println("Measurement sequence finished.");

  getMagPhase(realRcal, imageRcal, &magRcal, &phaseRcal);
  getMagPhase(realRz, imageRz, &magRz, &phaseRz);

  /* Finding the actual magnitude and phase */
  eis.magnitude = (magRcal / magRz) * _rcalVal; 
  eis.phaseRad = phaseRcal - phaseRz;
  eis.real = eis.magnitude * cos(eis.phaseRad);
  eis.imag = eis.magnitude * sin(eis.phaseRad) * -1; 
  eis.phaseDeg = eis.phaseRad * 180 / MATH_PI; 
  eis.freq = _currentFreq;

  /* Printing Values */
  printf("%d,", _sweepCfg.SweepIndex);
  printf("%.2f,", _currentFreq);
  printf("%.3f,", magRcal);
  printf("%.3f,", magRz);
  printf("%f,", eis.magnitude);
  printf("%.4f,", eis.real);
  printf("%.4f,", eis.imag);
  printf("%.4f\n", eis.phaseRad);

  eisArr[_sweepCfg.SweepIndex + (_currentCycle * _sweepCfg.SweepPoints)] = eis; 
  // printf("Array Index: %d\n",_sweepCfg.SweepIndex + (_currentCycle * _sweepCfg.SweepPoints));

  /* Updating Frequency */
  logSweep(&_sweepCfg, &_currentFreq);
}

void HELPStat::getDFT(int32_t* pReal, int32_t* pImage) 
{ 
  *pReal = AD5940_ReadAfeResult(AFERESULT_DFTREAL);
  *pReal &= 0x3ffff;
  /* Data is 18bit in two's complement, bit17 is the sign bit */
  if(*pReal&(1<<17)) *pReal |= 0xfffc0000;     

  delay(200); 

  *pImage = AD5940_ReadAfeResult(AFERESULT_DFTIMAGE);
  *pImage &= 0x3ffff;
  /* Data is 18bit in two's complement, bit17 is the sign bit */
  if(*pImage&(1<<17)) *pImage |= 0xfffc0000; 

  // printf("Real: %d, Image: %d\n", *pReal, *pImage);
}

void HELPStat::pollDFT(int32_t* pReal, int32_t* pImage)
{
  /* Polls the DFT and retrieves the real and imaginary data as ints */
  while(!AD5940_GetMCUIntFlag()) {
    delay(1000); // Adding an empirical delay before polling again 
  }
  
  if(AD5940_INTCTestFlag(AFEINTC_1,AFEINTSRC_DFTRDY)) {
    getDFT(pReal, pImage);
    delay(300);
    AD5940_ClrMCUIntFlag();
    AD5940_INTCClrFlag(AFEINTSRC_DFTRDY);
  }
  else Serial.println("Flag not working!");
}

void HELPStat::getMagPhase(int32_t real, int32_t image, float *pMag, float *pPhase)
{
  *pMag = sqrt((float)real*real + (float)image*image); 
  *pPhase =  atan2(-image, real);
  // printf("Magnitude: %f, Phase: %.4f\n", *pMag, *pPhase);
}

void HELPStat::logSweep(SoftSweepCfg_Type *pSweepCfg, float *pNextFreq) 
{
  float frequency; 

  // if you reach last point, go back to 0
  // if(++pSweepCfg->SweepIndex == pSweepCfg->SweepPoints) pSweepCfg->SweepIndex = 0;
  // if you reach last point, end the cycle
  if(++pSweepCfg->SweepIndex == pSweepCfg->SweepPoints) pSweepCfg -> SweepEn = bFALSE;
  else {
    frequency = pSweepCfg->SweepStart * pow(10, (pSweepCfg->SweepIndex * log10(pSweepCfg->SweepStop/pSweepCfg->SweepStart)/(pSweepCfg->SweepPoints-1)));
    *pNextFreq = frequency;

    /* Calibrating based on frequency */
    AD5940_WGFreqCtrlS(frequency, SYSCLCK);
    // checkFreq(frequency);
    configureFrequency(frequency);
  }
}

void HELPStat::runSweep(uint32_t numCycles, uint32_t delaySecs) 
{
  _numCycles = numCycles; 
  _currentCycle = 0; 
  /*
    Need to not run the program if ArraySize < total points 
    TO DO: ADD A CHECK HERE  
  */
  printf("Total points to run: %d\n", (_numCycles + 1) * _sweepCfg.SweepPoints); // since 0 based indexing, add 1
  printf("Set array size: %d\n", ARRAY_SIZE);
  printf("Calibration resistor value: %f\n", _rcalVal);

  // LED to show start of spectroscopy 
  // digitalWrite(LED1, HIGH); 

  for(uint32_t i = 0; i <= numCycles; i++) {
    /* 
      Wakeup AFE by read register, read 10 times at most.
      Do this because AD594x goes to sleep after each cycle. 
    */
    AD5940_SleepKeyCtrlS(SLPKEY_LOCK); // Disables Sleep Mode 
    if(delaySecs)
    {
      unsigned long prevTime = millis();
      unsigned long currTime = millis();
      printf("Delaying for %d seconds\n", delaySecs);
      while(currTime - prevTime < delaySecs * 1000)
      {
        currTime = millis();
        // printf("Curr delay: %d\n", currTime - prevTime);
      }
    } 
    // Timer for cycle time
    unsigned long timeStart = millis();

    if(i > 0){
      if(AD5940_WakeUp(10) > 10) Serial.println("Wakeup failed!");       
       resetSweep(&_sweepCfg, &_currentFreq);
       delay(300); // empirical settling delay
       _currentCycle++;
    }
    
    /* Calibrates based on frequency */
    // Should calibrate when AFE is active
    // checkFreq(_currentFreq);
    configureFrequency(_currentFreq);
    delay(10); // switching delay

    printf("Cycle %d\n", i);
    printf("Index, Frequency (Hz), DFT Cal, DFT Mag, Rz (Ohms), Rreal, Rimag, Rphase (rads)\n");
    
    while(_sweepCfg.SweepEn == bTRUE)
    {
      AD5940_DFTMeasure();
      // AD5940_DFTMeasureEIS();
      AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
              AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
              AFECTRL_SINC2NOTCH, bTRUE);
      delay(200);
    }

    unsigned long timeEnd = millis(); 
    printf("Time spent running Cycle %d (seconds): %lu\n", i, (timeEnd-timeStart)/1000);
  }
  
  /* Shutdown to conserve power. This turns off the LP-Loop and resets the AFE. */
  AD5940_ShutDownS();
  Serial.println("All cycles finished.");
  Serial.println("AD594x shutting down.");

  /* LEDs to show end of cycle */
  // digitalWrite(LED1, LOW);
  // digitalWrite(LED2, HIGH);
}

void HELPStat::resetSweep(SoftSweepCfg_Type *pSweepCfg, float *pNextFreq)
{
  /* Sets the index back to 0 and enables the sweep again */
  pSweepCfg->SweepIndex = 0; 
  pSweepCfg->SweepEn = bTRUE;

  /* Reset frequency back to start frequency */
  *pNextFreq = _startFreq; 
  AD5940_WGFreqCtrlS(_startFreq, SYSCLCK);
}

void HELPStat::settlingDelay(float freq) 
{
 
  // unsigned long constDelay = (4 * 1000 / freq); // delay constant just in case delay is too small
  unsigned long constDelay = 1000; // 1000 - delay constant just in case delay is too small 
   /* Getting the delay time based on frequency */
  if(freq <= 5) {
    constDelay = 2000; // 2000
    delay((unsigned long)(2 * 1000 / freq) + constDelay); // Trying 4 periods to improve Zw results instead of 2
  }
  else delay((constDelay)); 
  
}

AD5940Err HELPStat::checkFreq(float freq)
{
  /* 
    Adding a delay after recalibration to improve the switching noise.
    Sudden switching introduces inaccuracies, but accurate data I feel like
    is mostly dependent on the HSRTIA depending on the load impedance.
  */

  ADCFilterCfg_Type filter_cfg;
  DFTCfg_Type dft_cfg;
  HSDACCfg_Type hsdac_cfg;
  uint32_t WaitClks;
  ClksCalInfo_Type clks_cal;
  FreqParams_Type freq_params;

  float adcClck = 16e6;

  /* Step 1: Check Frequency */
  freq_params = AD5940_GetFreqParameters(freq);
  
       if(freq < 0.51)
	{
    /* Update HSDAC update rate */
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
     hsdac_cfg.HsDacUpdateRate = 0x1B;
    AD5940_HSDacCfgS(&hsdac_cfg);
    AD5940_HSRTIACfgS(HSTIARTIA_200);
    // AD5940_HSRTIACfgS(HSTIARTIA_10K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
    // AD5940_HSRTIACfgS(HSTIARTIA_80K);
    // AD5940_HSRTIACfgS(HSTIARTIA_160K);
	__AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    
    /* Change clock to 16MHz oscillator */
    AD5940_HPModeEn(bFALSE);
	}
        else if(freq < 1 )
	{
       /* Update HSDAC update rate */
    // hsdac_cfg.ExcitBufGain =EXCITBUFGAIN_2;// AppIMPCfg.ExcitBufGain;
    // hsdac_cfg.HsDacGain = HSDACGAIN_1;//AppIMPCfg.HsDacGain;
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
    hsdac_cfg.HsDacUpdateRate = 0x1B;
    AD5940_HSDacCfgS(&hsdac_cfg);
    AD5940_HSRTIACfgS(HSTIARTIA_200);
    // AD5940_HSRTIACfgS(HSTIARTIA_5K);
    // AD5940_HSRTIACfgS(HSTIARTIA_10K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
	__AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    
    /* Change clock to 16MHz oscillator */
    AD5940_HPModeEn(bFALSE);
    
	}

  else if(freq < 50)
	{
       /* Update HSDAC update rate */
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
    hsdac_cfg.HsDacUpdateRate = 0x1B;
    AD5940_HSDacCfgS(&hsdac_cfg);
    // AD5940_HSRTIACfgS(HSTIARTIA_200);
    AD5940_HSRTIACfgS(HSTIARTIA_1K);
    // AD5940_HSRTIACfgS(HSTIARTIA_5K);
    // AD5940_HSRTIACfgS(HSTIARTIA_10K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
  __AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    
    /* Change clock to 16MHz oscillator */
    AD5940_HPModeEn(bFALSE);
	}

  else if(freq < 400 )
	{
       /* Update HSDAC update rate */
    // hsdac_cfg.ExcitBufGain =EXCITBUFGAIN_2;// AppIMPCfg.ExcitBufGain;
    // hsdac_cfg.HsDacGain = HSDACGAIN_1;//AppIMPCfg.HsDacGain;
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
    hsdac_cfg.HsDacUpdateRate = 0x1B;
    AD5940_HSDacCfgS(&hsdac_cfg);
    // AD5940_HSRTIACfgS(HSTIARTIA_200);
    AD5940_HSRTIACfgS(HSTIARTIA_1K);
    // AD5940_HSRTIACfgS(HSTIARTIA_5K);
    // AD5940_HSRTIACfgS(HSTIARTIA_10K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
	__AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    
    /* Change clock to 16MHz oscillator */
    AD5940_HPModeEn(bFALSE);
    
	}

  else if(freq < 5000 )
	{
       /* Update HSDAC update rate */
    // hsdac_cfg.ExcitBufGain =EXCITBUFGAIN_2;// AppIMPCfg.ExcitBufGain;
    // hsdac_cfg.HsDacGain = HSDACGAIN_1;//AppIMPCfg.HsDacGain;
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
    hsdac_cfg.HsDacUpdateRate = 0x1B;
    AD5940_HSDacCfgS(&hsdac_cfg);
    AD5940_HSRTIACfgS(HSTIARTIA_200);
    // AD5940_HSRTIACfgS(HSTIARTIA_1K);
    // AD5940_HSRTIACfgS(HSTIARTIA_5K);
    // AD5940_HSRTIACfgS(HSTIARTIA_10K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
    // AD5940_HSRTIACfgS(HSTIARTIA_80K);
	__AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    
    /* Change clock to 16MHz oscillator */
    AD5940_HPModeEn(bFALSE);
    
	}

  else if(freq < 20000 )
	{
       /* Update HSDAC update rate */
    // hsdac_cfg.ExcitBufGain =EXCITBUFGAIN_2;// AppIMPCfg.ExcitBufGain;
    // hsdac_cfg.HsDacGain = HSDACGAIN_1;//AppIMPCfg.HsDacGain;
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
    hsdac_cfg.HsDacUpdateRate = 0x1B;
    AD5940_HSDacCfgS(&hsdac_cfg);
    AD5940_HSRTIACfgS(HSTIARTIA_200);
    // AD5940_HSRTIACfgS(HSTIARTIA_1K);
    // AD5940_HSRTIACfgS(HSTIARTIA_5K);
    // AD5940_HSRTIACfgS(HSTIARTIA_10K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
    // AD5940_HSRTIACfgS(HSTIARTIA_80K);
	__AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    
    /* Change clock to 16MHz oscillator */
    AD5940_HPModeEn(bFALSE);
    
	}
  
  else if(freq<80000)
       {
           /* Update HSDAC update rate */
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
    hsdac_cfg.HsDacUpdateRate = 0x1B;
    AD5940_HSDacCfgS(&hsdac_cfg);
    AD5940_HSRTIACfgS(HSTIARTIA_200);
    // AD5940_HSRTIACfgS(HSTIARTIA_1K);
    // AD5940_HSRTIACfgS(HSTIARTIA_5K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
    // AD5940_HSRTIACfgS(HSTIARTIA_80K);
	__AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    
    /* Change clock to 16MHz oscillator */
    AD5940_HPModeEn(bFALSE);
       }
        /* High power mode */
	if(freq >= 80000)
	{
		  /* Update HSDAC update rate */
    hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
    hsdac_cfg.HsDacGain = HSDACGAIN_0P2;
    hsdac_cfg.HsDacUpdateRate = 0x07;
    AD5940_HSDacCfgS(&hsdac_cfg);
    AD5940_HSRTIACfgS(HSTIARTIA_200);
    // AD5940_HSRTIACfgS(HSTIARTIA_1K);
    // AD5940_HSRTIACfgS(HSTIARTIA_5K);
    // AD5940_HSRTIACfgS(HSTIARTIA_20K);
    // AD5940_HSRTIACfgS(HSTIARTIA_40K);
    // AD5940_HSRTIACfgS(HSTIARTIA_80K);
	__AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
    
    /*Update ADC rate */
    filter_cfg.ADCRate = ADCRATE_1P6MHZ;
    adcClck = 32e6;
    
    /* Change clock to 32MHz oscillator */
    AD5940_HPModeEn(bTRUE);
	}
  
  /* Step 2: Adjust ADCFILTERCON and DFTCON to set optimumn SINC3, SINC2 and DFTNUM settings  */
  filter_cfg.ADCAvgNum = ADCAVGNUM_16;  /* Don't care because it's disabled */ 
  filter_cfg.ADCSinc2Osr = freq_params.ADCSinc2Osr;
  filter_cfg.ADCSinc3Osr = freq_params.ADCSinc3Osr;
  filter_cfg.BpSinc3 = bFALSE;
  filter_cfg.BpNotch = bTRUE;
  filter_cfg.Sinc2NotchEnable = bTRUE;
  dft_cfg.DftNum = freq_params.DftNum;
  dft_cfg.DftSrc = freq_params.DftSrc;
  dft_cfg.HanWinEn = bTRUE;
  AD5940_ADCFilterCfgS(&filter_cfg);
  AD5940_DFTCfgS(&dft_cfg);

  /* Step 3: Calculate clocks needed to get result to FIFO and update sequencer wait command */
  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = freq_params.DftSrc;
  clks_cal.DataCount = 1L<<(freq_params.DftNum+2); /* 2^(DFTNUMBER+2) */
  clks_cal.ADCSinc2Osr = freq_params.ADCSinc2Osr;
  clks_cal.ADCSinc3Osr = freq_params.ADCSinc3Osr;
  clks_cal.ADCAvgNum = 0;
  clks_cal.RatioSys2AdcClk = SYSCLCK/adcClck;
  AD5940_ClksCalculate(&clks_cal, &_waitClcks);

  return AD5940ERR_OK;
}

void HELPStat::sdWrite(char *output)
{
  File _file; 
  _file = SD.open(FILENAME, FILE_WRITE);
  if(_file)
  {
    Serial.println("File initialized. Writing data now...");
    _file.println(output); 
    Serial.println("Data written successfully.");
    _file.close(); 
  }
  else Serial.println("Unable to open the file.");

}

void HELPStat::sdAppend(char *output) 
{
  File _file; 
  _file = SD.open(FILENAME, FILE_APPEND);
  if(_file)
  {
    Serial.println("File initialized. Appending data now...");
    _file.println(output); 
    Serial.println("Data appended successfully.");
    _file.close(); 
  }
  else Serial.println("Unable to append to the file.");

}

void HELPStat::printData(void)
{
  // impStruct temp[_sweepCfg.SweepPoints];
  impStruct eis; 
  printf("Printing entire array now...\n");
  for(uint32_t i = 0; i <= _numCycles; i++)
  {
    printf("Cycle %d\n", i);
    printf("Index, Freq, Mag, Real, Imag, Phase (rad)\n");
    for(uint32_t j = 0; j < _sweepCfg.SweepPoints; j++)
    {
      eis = eisArr[j + (i * _sweepCfg.SweepPoints)];
      printf("%d,", j);
      printf("%.2f,", eis.freq);
      printf("%f,", eis.magnitude);
      printf("%.4f,", eis.real);
      printf("%.4f,", eis.imag);
      printf("%.4f\n", eis.phaseRad);
    }
  }
}

/*
  Surprised this works...maybe I overthought the whole SPI thing.
  Currently, the SPI configuration is set for 15 MHz, and from
  what I understand, the MicroSD card can take up to 40 MHz. So 
  I think as long as the configuration is within the SD card's range, 
  I can just use the existing SPI class rather than worrying about
  using the HSPI or VSPI (ESP32 uses VSPI ONLY by default when calling
  Arduino's SPI library).
*/

std::vector<float> HELPStat::calculateResistors(float rct_estimate, float rs_estimate) {
  std::vector<float> Z_real;
  std::vector<float> Z_imag;

  // Should append each real and imaginary data point
  for(uint32_t i = 0; i < _sweepCfg.SweepPoints; i++) {
    for(uint32_t j = 0; j <= _numCycles; j++) {
      impStruct eis;
      eis = eisArr[i + (j * _sweepCfg.SweepPoints)];
      Z_real.push_back(eis.real);
      Z_imag.push_back(eis.imag);
    }
  }

  float Rct = calculate_Rct(rct_estimate, rs_estimate, Z_real, Z_imag);
  float Rs  = calculate_Rs(rct_estimate, rs_estimate, Z_real, Z_imag);

  return({Rct, Rs});
}

void HELPStat::saveDataEIS(String dirName, String fileName)
{
  String directory = "/" + dirName;
  
  if(!SD.begin(CS_SD))
  {
    Serial.println("Card mount failed.");
    return;
  }

  if(!SD.exists(directory))
  {
    /* Creating a directory to store the data */
    if(SD.mkdir(directory)) Serial.println("Directory made successfully.");
    else
    {
      Serial.println("Couldn't make a directory or it already exists.");
      return;
    }
  }

  else Serial.println("Directory already exists.");

  /* Writing to the files */
  Serial.println("Saving to folder now.");

  // All cycles 
  String filePath = directory + "/" + fileName + ".csv";

  File dataFile = SD.open(filePath, FILE_WRITE); 
  if(dataFile)
  {
    for(uint32_t i = 0; i <= _numCycles; i++)
    {
      dataFile.print("Freq, Magnitude, Phase (rad), Phase (deg), Real, Imag");
    }
    dataFile.println("");
    for(uint32_t i = 0; i < _sweepCfg.SweepPoints; i++)
    {
      for(uint32_t j = 0; j <= _numCycles; j++)
      {
        impStruct eis;
        eis = eisArr[i + (j * _sweepCfg.SweepPoints)];
        dataFile.print(eis.freq);
        dataFile.print(",");
        dataFile.print(eis.magnitude);
        dataFile.print(",");
        dataFile.print(eis.phaseRad);
        dataFile.print(",");
        dataFile.print(eis.phaseDeg);
        dataFile.print(",");
        dataFile.print(eis.real);
        dataFile.print(",");
        dataFile.print(eis.imag);
        dataFile.print(",");
      }
      /* Moves to the next line */
      dataFile.println("");
    }
    dataFile.close();
    Serial.println("Data appended successfully.");
  }
}

void HELPStat::AD5940_BiasCfg(float startFreq, float endFreq, uint32_t numPoints, float biasVolt, float zeroVolt, int delaySecs) {

  // SETUP Cfgs
  CLKCfg_Type clk_cfg;
  AGPIOCfg_Type gpio_cfg;
  ClksCalInfo_Type clks_cal;
  
  // DFT / ADC / WG / HSLoop Cfgs
  AFERefCfg_Type aferef_cfg;
  HSLoopCfg_Type HsLoopCfg;
  DSPCfg_Type dsp_cfg;

  float sysClkFreq = 16000000.0; // 16 MHz
  float adcClkFreq = 16000000.0; // 16 MHz
  float sineVpp = 300; // 200 mV 

  /* Use hardware reset */
  AD5940_HWReset();
  AD5940_Initialize();

  /* Platform configuration */
  /* Step1. Configure clock - NEED THIS */
  clk_cfg.ADCClkDiv = ADCCLKDIV_1; // Clock source divider - ADC
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC; // Enables internal high frequency 16/32 MHz clock as source
  clk_cfg.SysClkDiv = SYSCLKDIV_1; // Clock source divider - System
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC; // Enables internal high frequency 16/32 MHz clock as source 
  clk_cfg.HfOSC32MHzMode = bFALSE; // Sets it to 16 MHz
  clk_cfg.HFOSCEn = bTRUE; // Enables the internal 16 / 32 MHz source
  clk_cfg.HFXTALEn = bFALSE; // Disables any need for external clocks
  clk_cfg.LFOSCEn = bTRUE; // Enables 32 kHz clock for timing / wakeups
  AD5940_CLKCfg(&clk_cfg); // Configures the clock
  Serial.println("Clock setup successfully.");

  /* Step3. Interrupt controller */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);   /* Enable all interrupt in INTC1, so we can check INTC flags */
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT); // Clears all INT flags

  /* Set INT0 source to be DFT READY */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DFTRDY, bTRUE); 
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT); // clears all flags 
  Serial.println("INTs setup successfully.");

  /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP0_INT;

  gpio_cfg.InputEnSet = 0; // Disables any GPIOs as inputs
  gpio_cfg.OutputEnSet = AGPIO_Pin0; // Enables GPIOs as outputs

  gpio_cfg.OutVal = 0; // Value for the output 
  gpio_cfg.PullEnSet = 0; // Disables any GPIO pull-ups / Pull-downs

  AD5940_AGPIOCfg(&gpio_cfg); // Configures the GPIOs
  Serial.println("GPIOs setup successfully.");

  /* CONFIGURING FOR DFT */
  // AFE Configuration 
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);  /* Initializing to disabled state */

  // Enabling high power bandgap since we're using High Power DAC
  // Enables operation at higher frequencies 
  aferef_cfg.HpBandgapEn = bTRUE;

  aferef_cfg.Hp1V1BuffEn = bTRUE; // Enables 1v1 buffer
  aferef_cfg.Hp1V8BuffEn = bTRUE; // Enables 1v8 buffer

  /* Not going to discharge capacitors - haven't seen this ever used */
  aferef_cfg.Disc1V1Cap = bFALSE;
  aferef_cfg.Disc1V8Cap = bFALSE;

  /* Disabling buffers and current limits*/
  aferef_cfg.Hp1V8ThemBuff = bFALSE;
  aferef_cfg.Hp1V8Ilimit = bFALSE;

  /* Disabling low power buffers */
  aferef_cfg.Lp1V1BuffEn = bFALSE;
  aferef_cfg.Lp1V8BuffEn = bFALSE;

  /* LP reference control - turn off if no bias */
  if(biasVolt != 0.0f)
  {
    aferef_cfg.LpBandgapEn = bTRUE;
    aferef_cfg.LpRefBufEn = bTRUE;
  }
  else
  {
    aferef_cfg.LpBandgapEn = bFALSE;
    aferef_cfg.LpRefBufEn = bFALSE;
  }
  aferef_cfg.LpRefBoostEn = bFALSE;

  AD5940_REFCfgS(&aferef_cfg);	// Configures the AFE 
  Serial.println("AFE setup successfully.");
  
  // Configuring High Speed Loop (high power loop)
  /* Vpp * BufGain * DacGain */
  HsLoopCfg.HsDacCfg.ExcitBufGain = EXCITBUFGAIN_2;
  HsLoopCfg.HsDacCfg.HsDacGain = HSDACGAIN_1;

  /* For low power / frequency measurements use 0x1B, o.w. 0x07 */
  HsLoopCfg.HsDacCfg.HsDacUpdateRate = 0x1B;
  HsLoopCfg.HsTiaCfg.DiodeClose = bFALSE;

  /* Assuming no bias - default to 1V1 bias */
  if(biasVolt != 0.0f) HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_VZERO0;
  else HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;

  /* Sets feedback capacitor on HSTIA */
  HsLoopCfg.HsTiaCfg.HstiaCtia = 31; /* 31pF + 2pF */

  /* No load and RTIA on the D Switch*/
  HsLoopCfg.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  HsLoopCfg.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_OPEN;

  /* Assuming low frequency measurement */
  HsLoopCfg.HsTiaCfg.HstiaRtiaSel = HSTIARTIA_40K;

  HsLoopCfg.SWMatCfg.Dswitch = SWD_CE0;       // Connects WG to CE0
  HsLoopCfg.SWMatCfg.Pswitch = SWP_RE0;       // Connects positive input to RE0
  HsLoopCfg.SWMatCfg.Nswitch = SWN_SE0;       // Connects negative input to SE0
  HsLoopCfg.SWMatCfg.Tswitch = SWT_SE0LOAD | SWT_TRTIA;   // Connects SEO to HSTIA via SE0Load

  HsLoopCfg.WgCfg.WgType = WGTYPE_SIN;
  
  /* Gain and offset calibration */
  HsLoopCfg.WgCfg.GainCalEn = bTRUE;
  HsLoopCfg.WgCfg.OffsetCalEn = bTRUE;

  _currentFreq = startFreq;
  HsLoopCfg.WgCfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(_currentFreq, sysClkFreq);
  HsLoopCfg.WgCfg.SinCfg.SinAmplitudeWord = (uint32_t)(sineVpp/800.0f*2047 + 0.5f);
  HsLoopCfg.WgCfg.SinCfg.SinOffsetWord = 0;
  HsLoopCfg.WgCfg.SinCfg.SinPhaseWord = 0;
  AD5940_HSLoopCfgS(&HsLoopCfg);
  Serial.println("HS Loop configured successfully");
  
  /* Configuring Sweep Functionality */
  _sweepCfg.SweepEn = bTRUE; 
  _sweepCfg.SweepLog = bTRUE;
  _sweepCfg.SweepIndex = 0; 
  _sweepCfg.SweepStart = startFreq; 
  _sweepCfg.SweepStop = endFreq;

  _startFreq = startFreq; 
  _endFreq = endFreq;
  
  // Defaulting to a logarithmic sweep. Works both upwards and downwards
  if(startFreq > endFreq) _sweepCfg.SweepPoints = (uint32_t)(1.5 + (log10(startFreq) - log10(endFreq)) * (numPoints)) - 1;
  else _sweepCfg.SweepPoints = (uint32_t)(1.5 + (log10(endFreq) - log10(startFreq)) * (numPoints)) - 1;
  printf("Number of points: %d\n", _sweepCfg.SweepPoints);
  Serial.println("Sweep configured successfully.");

  /* Configuring LPDAC if necessary */
  if(biasVolt != 0.0f)
  {
    LPDACCfg_Type lpdac_cfg;
    
    lpdac_cfg.LpdacSel = LPDAC0;
    lpdac_cfg.LpDacVbiasMux = LPDACVBIAS_12BIT; /* Use Vbias to tune BiasVolt. */
    lpdac_cfg.LpDacVzeroMux = LPDACVZERO_6BIT;  /* Vbias-Vzero = BiasVolt */

    // Uses 2v5 as a reference, can set to AVDD
    lpdac_cfg.LpDacRef = LPDACREF_2P5;
    lpdac_cfg.LpDacSrc = LPDACSRC_MMR;      /* Use MMR data, we use LPDAC to generate bias voltage for LPTIA - the Vzero */
    lpdac_cfg.PowerEn = bTRUE;              /* Power up LPDAC */

    /*
      Note on bias: 
      Examples from AD differ based on impedance and EIS versions of code. 
      Default version is centered around 1.1V for Vzero with no compensation as per datasheet.
      That being said, still getting relatively accurate results for Vbias - Vzero. 
      So, it works decently well. Error in bias voltage increases with the bias though, I found.

      Custom Vzero version from EIS has slightly different formula than datasheet I believe, 
      so I tried my best to find the best of both worlds for the custom one to follow both datasheet
      and AD example. Recommend sticking with default version for testing. 
    */

    /* 
      Default bias case - centered around Vzero = 1.1V
      This works decently well. Error seems to increase as you go higher in bias.
     */
    if(!zeroVolt)
    {
      // Edge cases 
      if(biasVolt<-1100.0f) biasVolt = -1100.0f + DAC12BITVOLT_1LSB;
      if(biasVolt> 1100.0f) biasVolt = 1100.0f - DAC12BITVOLT_1LSB;
      
      /* Bit conversion from voltage */
      // Converts the bias voltage to a data bit - uses the 1100 to offset it with Vzero
      lpdac_cfg.DacData6Bit = 0x40 >> 1;            /* Set Vzero to middle scale - sets Vzero to 1.1V */
      lpdac_cfg.DacData12Bit = (uint16_t)((biasVolt + 1100.0f)/DAC12BITVOLT_1LSB);
    }
    else
    {
      /* 
        Seems to work well. VBIAS needs to be set relative to VZERO. 
        i.e. let VBIAS be the bias voltage you want (100 mV, 20 mV, etc.) 
        and set VZERO to a constant.
     
      */
      lpdac_cfg.DacData6Bit = (uint32_t)((zeroVolt-200)/DAC6BITVOLT_1LSB);
      lpdac_cfg.DacData12Bit = (int32_t)((biasVolt)/DAC12BITVOLT_1LSB) + (lpdac_cfg.DacData6Bit * 64);
      if(lpdac_cfg.DacData12Bit < lpdac_cfg.DacData6Bit * 64) lpdac_cfg.DacData12Bit--; // compensation as per datasheet 
    }

    lpdac_cfg.DataRst = bFALSE;      /* Do not reset data register */
    // Allows for measuring of Vbias and Vzero voltages and connects them to LTIA, LPPA, and HSTIA
    lpdac_cfg.LpDacSW = LPDACSW_VBIAS2LPPA|LPDACSW_VBIAS2PIN|LPDACSW_VZERO2LPTIA|LPDACSW_VZERO2PIN|LPDACSW_VZERO2HSTIA;
    AD5940_LPDACCfgS(&lpdac_cfg);
    Serial.println("LPDAC configured successfully.");
  }

  /* Sets the input of the ADC to the output of the HSTIA */
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_HSTIA_N;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_HSTIA_P;

  /* Programmable gain array for the ADC */
  dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_1;
  
  /* Disables digital comparator functionality */
  memset(&dsp_cfg.ADCDigCompCfg, 0, sizeof(dsp_cfg.ADCDigCompCfg));
  
  /* Is this actually being used? */
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_16; // Impedance example uses 16 
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;	/* Tell filter block clock rate of ADC*/

  // dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_1P6MHZ;	/* Tell filter block clock rate of ADC*/
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = ADCSINC2OSR_22; // Oversampling ratio for SINC2
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_2; // Oversampling ratio for SINC3
  /* Using Recommended OSR of 4 for SINC3 */
  // dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_4; // Oversampling ratio for SINC3

  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE; // Bypasses Notch filter
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE; // Doesn't bypass SINC3
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bTRUE; // Enables SINC2 filter

  dsp_cfg.DftCfg.DftNum = DFTNUM_16384; // Max number of DFT points
  dsp_cfg.DftCfg.DftSrc = DFTSRC_SINC3; // Sets DFT source to SINC3
  dsp_cfg.DftCfg.HanWinEn = bTRUE;  // Enables HANNING WINDOW - recommended to always be on 
  
  /* Disables STAT block */
  memset(&dsp_cfg.StatCfg, 0, sizeof(dsp_cfg.StatCfg));
  
  AD5940_DSPCfgS(&dsp_cfg); // Sets the DFT 
  Serial.println("DSP configured successfully.");

  /* Calculating Clock Cycles to wait given DFT settings */
  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = DFTSRC_SINC3; // Source of DFT
  clks_cal.DataCount = 1L<<(DFTNUM_16384+2); /* 2^(DFTNUMBER+2) */
  clks_cal.ADCSinc2Osr = ADCSINC2OSR_22;
  clks_cal.ADCSinc3Osr = ADCSINC3OSR_2;
  clks_cal.ADCAvgNum = ADCAVGNUM_16;
  clks_cal.RatioSys2AdcClk = sysClkFreq / adcClkFreq; // Same ADC / SYSTEM CLCK FREQ
  AD5940_ClksCalculate(&clks_cal, &_waitClcks);

  /* Clears any interrupts just in case */
  AD5940_ClrMCUIntFlag();
  AD5940_INTCClrFlag(AFEINTSRC_DFTRDY);

  // Added bias option conditionally 
  if(biasVolt == 0.0f)
  {
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bTRUE);
    Serial.println("No bias applied.");
  }
  else
  {
    /* 
      Also powers the DC offset buffers that's used with LPDAC (Vbias) 
      Buffers need to be powered up here but aren't turned off in measurement like 
      the rest. This is to ensure the LPDAC and bias stays on the entire time.
    */
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
              AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
              AFECTRL_SINC2NOTCH|AFECTRL_DCBUFPWR, bTRUE);
    Serial.println("Bias is applied.");
  }

  Serial.println("Everything turned on.");
  printf("Testing bias voltage: Vbias - Vzero =  %f\n", biasVolt);
}

void HELPStat::AD5940_DFTMeasureEIS(void) {

  SWMatrixCfg_Type sw_cfg;
  LPAmpCfg_Type LpAmpCfg;
  impStruct eis;

  /* Real / Imaginary components */
  int32_t realRcal, imageRcal; 
  int32_t realRload, imageRload; 
  int32_t realRzRload, imageRzRload; 

  // float rcalVal = 10030; // known Rcal

  /* Using fImpCar struct from AD */
  fImpCar_Type rCal, rzRload, rLoad;
  fImpCar_Type DftConst1 = {1.0f, 0}; // needed for floating point math
  fImpCar_Type temp1, temp2, res; // placeholder values

  AD5940_Delay10us(_waitClcks * (1/SYSCLCK));

  /* Disconnect SE0 from LPTIA*/
	// LpAmpCfg.LpAmpPwrMod = LPAMPPWR_NORM;
  // LpAmpCfg.LpPaPwrEn = bTRUE;
  // LpAmpCfg.LpTiaPwrEn = bTRUE;
  // LpAmpCfg.LpTiaRf = LPTIARF_1M;
  // LpAmpCfg.LpTiaRload = LPTIARLOAD_100R;
  // LpAmpCfg.LpTiaRtia = LPTIARTIA_OPEN; /* Disconnect Rtia to avoid RC filter discharge */
  // LpAmpCfg.LpTiaSW = LPTIASW(7)|LPTIASW(8)|LPTIASW(12)|LPTIASW(13); 
	// AD5940_LPAMPCfgS(&LpAmpCfg);
  // delay(100);

  /* Measuring Rload and Rz */
  sw_cfg.Dswitch = SWD_CE0;
  sw_cfg.Pswitch = SWP_RE0;
  sw_cfg.Nswitch = SWN_SE0;
  sw_cfg.Tswitch = SWT_TRTIA|SWT_SE0LOAD;
  AD5940_SWMatrixCfgS(&sw_cfg);
  // delay(100); // empirical delay for switching 

  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_WG, bTRUE);  /* Enable Waveform generator */
  // delay(500);
  settlingDelay(_currentFreq);


  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);  /* Start ADC convert and DFT */
  settlingDelay(_currentFreq);

  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));

  /* Polling and retrieving data from the DFT */
  pollDFT(&realRzRload, &imageRzRload);
  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG, bFALSE);  /* Stop ADC convert and DFT */

   /* Measuring Rload */
  sw_cfg.Dswitch = SWD_SE0;
  sw_cfg.Pswitch = SWP_SE0;
  sw_cfg.Nswitch = SWN_SE0LOAD;
  sw_cfg.Tswitch = SWT_SE0LOAD|SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
	// delay(100); // empirical delay for switching 

  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);  /* Enable Waveform generator */
  // delay(500);
  settlingDelay(_currentFreq);

  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);  /* Start ADC convert and DFT */
  settlingDelay(_currentFreq);

  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));

  /* Polling and retrieving data from the DFT */
  pollDFT(&realRload, &imageRload);

  /* Shutting off ADC and AFE */
  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG|AFECTRL_ADCPWR, bFALSE);  /* Stop ADC convert and DFT */

  /* Measuring RCAL */
  sw_cfg.Dswitch = SWD_RCAL0;
  sw_cfg.Pswitch = SWP_RCAL0;
  sw_cfg.Nswitch = SWN_RCAL1;
  sw_cfg.Tswitch = SWT_RCAL1|SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);
	// delay(100); // empirical delay for switching 

  AD5940_AFECtrlS(AFECTRL_WG|AFECTRL_ADCPWR, bTRUE);  /* Enable Waveform generator */
  // delay(500);
  settlingDelay(_currentFreq);

  AD5940_AFECtrlS(AFECTRL_ADCCNV|AFECTRL_DFT, bTRUE);  /* Start ADC convert and DFT */
  settlingDelay(_currentFreq);
  
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));
  AD5940_Delay10us((_waitClcks / 2) * (1/SYSCLCK));

  /* Polling and retrieving data from the DFT */
  pollDFT(&realRcal, &imageRcal);

  //wait for first data ready
  AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_ADCCNV|AFECTRL_DFT|AFECTRL_WG, bFALSE);  /* Stop ADC convert and DFT */
  AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                AFECTRL_SINC2NOTCH, bFALSE);

  /* Type converting to floats and assigning to fImp structs */
  rCal.Real = (float) realRcal;
  rCal.Image = (float) -imageRcal;
  rzRload.Real = (float) realRzRload;
  rzRload.Image = (float) -imageRzRload;
  rLoad.Real = (float) realRload;
  rLoad.Image = (float) -imageRload;

  // /* Calculating magnitude and phase */
  // temp1 = AD5940_ComplexSubFloat(&rLoad, &rzRload);
  // temp2 = AD5940_ComplexMulFloat(&rzRload, &rLoad);
  // res = AD5940_ComplexDivFloat(&temp1, &temp2);
  // res = AD5940_ComplexMulFloat(&rCal, &res);

  temp1 = AD5940_ComplexDivFloat(&DftConst1, &rzRload); 
  temp2 = AD5940_ComplexDivFloat(&DftConst1, &rLoad); 
  res = AD5940_ComplexSubFloat(&temp1, &temp2);
  res = AD5940_ComplexMulFloat(&res, &rCal);

  /* Finding the actual magnitude and phase */
  eis.magnitude = AD5940_ComplexMag(&res) * _rcalVal; 

  // phase calculations: 
  // eis.phaseRad = AD5940_ComplexPhase(&rCal) + AD5940_ComplexPhase(&temp1) - AD5940_ComplexPhase(&rzRload) - AD5940_ComplexPhase(&rLoad);
  
  eis.phaseRad = AD5940_ComplexPhase(&res);
  eis.real = eis.magnitude * cos(eis.phaseRad);
  eis.imag = eis.magnitude * sin(eis.phaseRad) * -1; 
  eis.phaseDeg = eis.phaseRad * 180 / MATH_PI; 
  eis.freq = _currentFreq;

  /* Printing Values */
  printf("%d,", _sweepCfg.SweepIndex);
  printf("%.2f,", _currentFreq);
  printf("%.2f", AD5940_ComplexMag(&rzRload));
  printf("%.2f", AD5940_ComplexMag(&rLoad));
  printf("%f,", eis.magnitude);
  printf("%.4f,", eis.real);
  printf("%.4f,", eis.imag);
  printf("%.4f\n", eis.phaseRad);
  // printf("rLoad: %.3f,", AD5940_ComplexMag(&rLoad));
  // printf("rzRload: %.3f\n", AD5940_ComplexMag(&rzRload));
  // printf("rLoad phase: %.3f,", AD5940_ComplexPhase(&rLoad));
  // printf("rzRload: %.3f\n", AD5940_ComplexPhase(&rzRload));

  /* Saving to an array */
  eisArr[_sweepCfg.SweepIndex + (_currentCycle * _sweepCfg.SweepPoints)] = eis; 

  /* Updating Frequency */
  logSweep(&_sweepCfg, &_currentFreq);
}

/* Helper functions for refactoring how checkFreq works */
// Adapted from AD5940 Impedance Examples
void HELPStat::configureDFT(float freq)
{
  FreqParams_Type freq_params;
  ClksCalInfo_Type clks_cal;
  ADCFilterCfg_Type filter_cfg;
  DFTCfg_Type dft_cfg;
  float adcClck; 

  /* Getting optimal freq parameters */
  freq_params = AD5940_GetFreqParameters(freq);

  /* Adjusting Filter and DFT configurations using
  optimal settings (as per AD5940 library)*/

  if(freq >= 80000)
  {
    filter_cfg.ADCRate = ADCRATE_1P6MHZ;
    adcClck = 32e6;
    AD5940_HPModeEn(bTRUE); // High Power Mode
    // Serial.println("HP Mode On");
  }
  else
  {
    filter_cfg.ADCRate = ADCRATE_800KHZ;
    adcClck = 16e6;
    AD5940_HPModeEn(bFALSE); // Low Power Mode
    // Serial.println("LP Mode On");
  }

  filter_cfg.ADCAvgNum = ADCAVGNUM_16;  // Not using this so it doesn't matter 
  filter_cfg.ADCSinc2Osr = freq_params.ADCSinc2Osr;
  filter_cfg.ADCSinc3Osr = freq_params.ADCSinc3Osr;
  filter_cfg.BpSinc3 = bFALSE;
  filter_cfg.BpNotch = bTRUE; // Not using onboard 60 Hz Notch Filter
  filter_cfg.Sinc2NotchEnable = bTRUE;
  
  dft_cfg.DftNum = freq_params.DftNum;
  dft_cfg.DftSrc = freq_params.DftSrc;
  dft_cfg.HanWinEn = bTRUE;
  // Serial.println("Filter and DFT configured.");

  AD5940_ADCFilterCfgS(&filter_cfg);
  AD5940_DFTCfgS(&dft_cfg);

  /* Calculating clock cycles - usually used for FIFO but I use it here
  as an additional wait time calculator. Potentially redundant, but keeping it for now.*/
  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = freq_params.DftSrc;
  clks_cal.DataCount = 1L<<(freq_params.DftNum+2); /* 2^(DFTNUMBER+2) */
  clks_cal.ADCSinc2Osr = freq_params.ADCSinc2Osr;
  clks_cal.ADCSinc3Osr = freq_params.ADCSinc3Osr;
  clks_cal.ADCAvgNum = 0;
  clks_cal.RatioSys2AdcClk = SYSCLCK/adcClck;
  AD5940_ClksCalculate(&clks_cal, &_waitClcks);
  // Serial.println("Clocks calculated.");
}

AD5940Err HELPStat::setHSTIA(float freq)
{
  HSDACCfg_Type hsdac_cfg;
  FreqParams_Type freq_params;
  ClksCalInfo_Type clks_cal;
  ADCFilterCfg_Type filter_cfg;
  DFTCfg_Type dft_cfg;
  float adcClck; 

  AD5940Err exitStatus = AD5940ERR_ERROR;

  hsdac_cfg.ExcitBufGain = _extGain;
  hsdac_cfg.HsDacGain = _dacGain;

  // hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_0P25;
  // hsdac_cfg.HsDacGain = HSDACGAIN_0P2;

  // hsdac_cfg.ExcitBufGain = EXCITBUFGAIN_2;
  // hsdac_cfg.HsDacGain = HSDACGAIN_1;

  /* Getting optimal freq parameters */
  freq_params = AD5940_GetFreqParameters(freq);

  // Running through the frequencies and tuning to the specified RTIA
  // It checks if the frequency is less than the cutoff
  // This method works only if the lowest frequency in the sweep is 
  // included. Otherwise you have a gap in calibration ranges.

  for(uint32_t i = 0; i < _gainArrSize; i++)
  {
    if(freq <= _gainArr[i].freq) 
    {
      if(freq >= 80000) // High Power Mode Case
      {
        hsdac_cfg.HsDacUpdateRate = 0x07;
        AD5940_HSDacCfgS(&hsdac_cfg);
        AD5940_HSRTIACfgS(_gainArr[i].rTIA);
      __AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);
        
        // AD5940_HPModeEn(bTRUE);
        // printf("Setting HSTIA to: %d for %.2f Hz\n", _gainArr[i].rTIA, freq);

        filter_cfg.ADCRate = ADCRATE_1P6MHZ;
        adcClck = 32e6;
        AD5940_HPModeEn(bTRUE);
        exitStatus = AD5940ERR_OK;
        break;

        // return AD5940ERR_OK;
      }
      else
      {
        hsdac_cfg.HsDacUpdateRate = 0x1B;
        AD5940_HSDacCfgS(&hsdac_cfg);
        AD5940_HSRTIACfgS(_gainArr[i].rTIA);
      __AD5940_SetDExRTIA(0, HSTIADERTIA_OPEN, HSTIADERLOAD_0R);

        // AD5940_HPModeEn(bFALSE);
        // printf("Setting HSTIA to: %d for %.2f Hz\n", _gainArr[i].rTIA, freq);

        filter_cfg.ADCRate = ADCRATE_800KHZ;
        adcClck = 16e6;
        AD5940_HPModeEn(bFALSE); // Low Power Mode
        exitStatus = AD5940ERR_OK;
        break;

        // return AD5940ERR_OK;
      }
    }
  }
  // Serial.println("No longer in for loop!");
  filter_cfg.ADCAvgNum = ADCAVGNUM_16;  // Not using this so it doesn't matter 
  filter_cfg.ADCSinc2Osr = freq_params.ADCSinc2Osr;
  filter_cfg.ADCSinc3Osr = freq_params.ADCSinc3Osr;
  filter_cfg.BpSinc3 = bFALSE;
  filter_cfg.BpNotch = bTRUE; // Not using onboard 60 Hz Notch Filter
  filter_cfg.Sinc2NotchEnable = bTRUE;
  
  dft_cfg.DftNum = freq_params.DftNum;
  dft_cfg.DftSrc = freq_params.DftSrc;
  dft_cfg.HanWinEn = bTRUE;
  // Serial.println("Filter and DFT configured.");

  AD5940_ADCFilterCfgS(&filter_cfg);
  AD5940_DFTCfgS(&dft_cfg);

  /* Calculating clock cycles - usually used for FIFO but I use it here
  as an additional wait time calculator. Potentially redundant, but keeping it for now.*/
  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = freq_params.DftSrc;
  clks_cal.DataCount = 1L<<(freq_params.DftNum+2); /* 2^(DFTNUMBER+2) */
  clks_cal.ADCSinc2Osr = freq_params.ADCSinc2Osr;
  clks_cal.ADCSinc3Osr = freq_params.ADCSinc3Osr;
  clks_cal.ADCAvgNum = 0;
  clks_cal.RatioSys2AdcClk = SYSCLCK/adcClck;
  AD5940_ClksCalculate(&clks_cal, &_waitClcks);
  // Serial.println("Clocks calculated.");
  // printf("Calibrated for freq: %.2f\n", freq);
  // If we can't calibrate based on configuration, return an error.
  
  return exitStatus;
}

void HELPStat::configureFrequency(float freq)
{
  AD5940Err check = setHSTIA(freq);
  // configureDFT(freq);

  if(check != AD5940ERR_OK) Serial.println("Unable to configure.");
  // else Serial.println("Configured successfully.");
}

/* Current noise measurements */
float HELPStat::getADCVolt(uint32_t gainPGA, float vRef1p82) 
{ 
  /* Bypassing SINC3 gets us ADC data */
  uint32_t adcCode = AD5940_ReadAfeResult(AFERESULT_SINC3);
  float vOut = AD5940_ADCCode2Volt(adcCode, gainPGA, vRef1p82);
}

float HELPStat::pollADC(uint32_t gainPGA, float vRef1p82)
{
  float vOut;
  float kFactor = 1.835/1.82;
  /* Polls ADC and returns a voltage and the ADCcode */
  while(!AD5940_GetMCUIntFlag()) {
    // Serial.println("Delaying polling!");
    AD5940_Delay10us(200);
  }
  Serial.println("reading ADC now!");
  // if(AD5940_INTCTestFlag(AFEINTC_1,AFEINTSRC_SINC2RDY)) {
  //   vOut = getADCVolt(adcCode, gainPGA, vRef1p82);
  //   // Serial.println("Just read the ADC voltage!");
  //   AD5940_Delay10us(100);
  //   AD5940_ClrMCUIntFlag();
  //   AD5940_INTCClrFlag(AFEINTSRC_ADCRDY);
  //   return vOut;
  // }
  if(AD5940_INTCTestFlag(AFEINTC_0,AFEINTSRC_SINC2RDY))  
    {
      Serial.println("Reading AFE!");
      static uint32_t count;
      AD5940_INTCClrFlag(AFEINTSRC_SINC2RDY);
      int32_t rd = AD5940_ReadAfeResult(AFERESULT_SINC2);
      count ++;
      /* ADC Sample rate is 800kSPS. SINC3 OSR is 4, SINC2 OSR is 1333. So the final output data rate is 800kSPS/4/1333 = 150.0375Hz */
      if(count == 150) /* Print data @1Hz */
      {
        count = 0;
        float temp = rd - 32768;
        float diff_volt = temp * vRef1p82 / 32768 * kFactor;
        printf("ADC Code:%d, diff-volt: %.4f, volt:%.4f\n",rd, diff_volt, diff_volt+1.11);
      }
    }
  else Serial.println("Flag not working!");
}

void HELPStat::AD5940_TDDNoise(float biasVolt, float zeroVolt) {

  // SETUP Cfgs
  CLKCfg_Type clk_cfg;
  AGPIOCfg_Type gpio_cfg;
  ClksCalInfo_Type clks_cal;
  LPAmpCfg_Type LpAmpCfg;
  
  // DFT / ADC / WG / HSLoop Cfgs
  AFERefCfg_Type aferef_cfg;
  HSLoopCfg_Type HsLoopCfg;
  DSPCfg_Type dsp_cfg;

  float sysClkFreq = 16000000.0; // 16 MHz
  float adcClkFreq = 16000000.0; // 16 MHz
  float sineVpp = 200.0; // 200 mV 

  /* Use hardware reset */
  AD5940_HWReset();
  AD5940_Initialize();

  /* Platform configuration */
  /* Step1. Configure clock - NEED THIS */
  clk_cfg.ADCClkDiv = ADCCLKDIV_1; // Clock source divider - ADC
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC; // Enables internal high frequency 16/32 MHz clock as source
  clk_cfg.SysClkDiv = SYSCLKDIV_1; // Clock source divider - System
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC; // Enables internal high frequency 16/32 MHz clock as source 
  clk_cfg.HfOSC32MHzMode = bFALSE; // Sets it to 16 MHz
  clk_cfg.HFOSCEn = bTRUE; // Enables the internal 16 / 32 MHz source
  clk_cfg.HFXTALEn = bFALSE; // Disables any need for external clocks
  clk_cfg.LFOSCEn = bTRUE; // Enables 32 kHz clock for timing / wakeups
  AD5940_CLKCfg(&clk_cfg); // Configures the clock
  Serial.println("Clock setup successfully.");

  /* Step3. Interrupt controller */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);   /* Enable all interrupt in INTC1, so we can check INTC flags */
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT); // Clears all INT flags

  /* Set INT0 source to be ADC READY */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_SINC2RDY, bTRUE); 
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT); // clears all flags 
  Serial.println("INTs setup successfully.");

  /* Step4: Reconfigure GPIO */
  gpio_cfg.FuncSet = GP0_INT;

  gpio_cfg.InputEnSet = 0; // Disables any GPIOs as inputs
  gpio_cfg.OutputEnSet = AGPIO_Pin0; // Enables GPIOs as outputs

  gpio_cfg.OutVal = 0; // Value for the output 
  gpio_cfg.PullEnSet = 0; // Disables any GPIO pull-ups / Pull-downs

  AD5940_AGPIOCfg(&gpio_cfg); // Configures the GPIOs
  Serial.println("GPIOs setup successfully.");

  /* CONFIGURING FOR DFT */
  // AFE Configuration 
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);  /* Initializing to disabled state */

  // Enabling high power bandgap since we're using High Power DAC
  // Enables operation at higher frequencies 
  aferef_cfg.HpBandgapEn = bTRUE;

  aferef_cfg.Hp1V1BuffEn = bTRUE; // Enables 1v1 buffer
  aferef_cfg.Hp1V8BuffEn = bTRUE; // Enables 1v8 buffer

  /* Not going to discharge capacitors - haven't seen this ever used */
  aferef_cfg.Disc1V1Cap = bFALSE;
  aferef_cfg.Disc1V8Cap = bFALSE;

  /* Disabling buffers and current limits*/
  aferef_cfg.Hp1V8ThemBuff = bFALSE;
  aferef_cfg.Hp1V8Ilimit = bFALSE;

  /* Disabling low power buffers */
  aferef_cfg.Lp1V1BuffEn = bFALSE;
  aferef_cfg.Lp1V8BuffEn = bFALSE;

  /* LP reference control - turn off if no bias */
  if((biasVolt == 0.0f) && (zeroVolt == 0.0f))
  {
    aferef_cfg.LpBandgapEn = bFALSE;
    aferef_cfg.LpRefBufEn = bFALSE;
    printf("No bias today!\n");
  }
  else
  {
    aferef_cfg.LpBandgapEn = bTRUE;
    aferef_cfg.LpRefBufEn = bTRUE;
    printf("We have bias!\n");
  }

  /* Doesn't enable boosting buffer current */
  aferef_cfg.LpRefBoostEn = bFALSE;
  AD5940_REFCfgS(&aferef_cfg);	// Configures the AFE 
  Serial.println("AFE setup successfully.");
  
  /* Disconnect SE0 from LPTIA - double check this too */
	LpAmpCfg.LpAmpPwrMod = LPAMPPWR_NORM;
  LpAmpCfg.LpPaPwrEn = bFALSE; //bTRUE
  LpAmpCfg.LpTiaPwrEn = bFALSE; //bTRUE
  LpAmpCfg.LpTiaRf = LPTIARF_1M;
  LpAmpCfg.LpTiaRload = LPTIARLOAD_100R;
  LpAmpCfg.LpTiaRtia = LPTIARTIA_OPEN; /* Disconnect Rtia to avoid RC filter discharge */
  LpAmpCfg.LpTiaSW = LPTIASW(7)|LPTIASW(8)|LPTIASW(12)|LPTIASW(13); 
	AD5940_LPAMPCfgS(&LpAmpCfg);
  Serial.println("SE0 disconnected from LPTIA.");
  
  // Configuring High Speed Loop (high power loop)
  /* Vpp * BufGain * DacGain */
  /*
    Configuration shouldn't matter because CE0 is disconnected
    but I set it anyways since not sure if I can leave it uninitialized. 
  */
  HsLoopCfg.HsDacCfg.ExcitBufGain = EXCITBUFGAIN_2;
  HsLoopCfg.HsDacCfg.HsDacGain = HSDACGAIN_1;

  /* For low power / frequency measurements use 0x1B, o.w. 0x07 */
  HsLoopCfg.HsDacCfg.HsDacUpdateRate = 0x1B;
  HsLoopCfg.HsTiaCfg.DiodeClose = bFALSE;

  /* Assuming no bias - default to 1V1 bias */
  if((biasVolt == 0.0f) && (zeroVolt == 0.0f))
  {
    HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_1P1;
    printf("HSTIA bias set to 1.1V.\n");
  }
  else 
  {
    HsLoopCfg.HsTiaCfg.HstiaBias = HSTIABIAS_VZERO0;
    printf("HSTIA bias set to Vzero.\n");
  }

  /* Sets feedback capacitor on HSTIA */
  HsLoopCfg.HsTiaCfg.HstiaCtia = 31; /* 31pF + 2pF */

  /* No load and RTIA on the D Switch*/
  HsLoopCfg.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN;
  HsLoopCfg.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_OPEN;

  /* Assuming low frequency measurement */
  HsLoopCfg.HsTiaCfg.HstiaRtiaSel = HSTIARTIA_10K;

  HsLoopCfg.SWMatCfg.Dswitch = SWD_CE0;       // Connects WG to CE0
  HsLoopCfg.SWMatCfg.Pswitch = SWP_RE0;       // Connects positive input to RE0
  HsLoopCfg.SWMatCfg.Nswitch = SWN_SE0;       // Connects negative input to SE0
  HsLoopCfg.SWMatCfg.Tswitch = SWT_SE0LOAD|SWT_TRTIA;   // Connects SEO to HSTIA via SE0Load

  /*
    Shouldn't matter since WG is going to be connected to CE0. 
    Set to not leave unitialized.
  */
  // memset(&HSLoopCfg.WgCfg, 0, sizeof(HSLoopCfg.WgCfg));

  HsLoopCfg.WgCfg.WgType = WGTYPE_SIN;
  HsLoopCfg.WgCfg.GainCalEn = bTRUE;          // Gain calibration
  HsLoopCfg.WgCfg.OffsetCalEn = bTRUE;        // Offset calibration
  printf("Current Freq: %f\n", 100);
  HsLoopCfg.WgCfg.SinCfg.SinFreqWord = AD5940_WGFreqWordCal(100, sysClkFreq);
  HsLoopCfg.WgCfg.SinCfg.SinAmplitudeWord = (uint32_t)((sineVpp/800.0f)*2047 + 0.5f);
  HsLoopCfg.WgCfg.SinCfg.SinOffsetWord = 0;
  HsLoopCfg.WgCfg.SinCfg.SinPhaseWord = 0;
  AD5940_HSLoopCfgS(&HsLoopCfg);
  Serial.println("HS Loop configured successfully");
  
  /* Configuring Sweep Functionality - doesn't matter here */
  _sweepCfg.SweepEn = bFALSE; 
  _sweepCfg.SweepLog = bTRUE;
  _sweepCfg.SweepIndex = 0; 
  _sweepCfg.SweepStart = 1000; 
  _sweepCfg.SweepStop = 10;
  _sweepCfg.SweepPoints = 5;

   /* Configuring LPDAC if necessary */
  if((biasVolt != 0.0f) || (zeroVolt != 0.0f))
  {
    LPDACCfg_Type lpdac_cfg;
    
    lpdac_cfg.LpdacSel = LPDAC0;
    lpdac_cfg.LpDacVbiasMux = LPDACVBIAS_12BIT; /* Use Vbias to tune BiasVolt. */
    lpdac_cfg.LpDacVzeroMux = LPDACVZERO_6BIT;  /* Vbias-Vzero = BiasVolt */

    // Uses 2v5 as a reference, can set to AVDD
    lpdac_cfg.LpDacRef = LPDACREF_2P5;
    lpdac_cfg.LpDacSrc = LPDACSRC_MMR;      /* Use MMR data, we use LPDAC to generate bias voltage for LPTIA - the Vzero */
    lpdac_cfg.PowerEn = bTRUE;              /* Power up LPDAC */
    /* 
      Default bias case - centered around Vzero = 1.1V
      This works decently well. Error seems to increase as you go higher in bias.
     */
    if(zeroVolt == 0.0f)
    {
      // Edge cases 
      if(biasVolt<-1100.0f) biasVolt = -1100.0f + DAC12BITVOLT_1LSB;
      if(biasVolt> 1100.0f) biasVolt = 1100.0f - DAC12BITVOLT_1LSB;
      
      /* Bit conversion from voltage */
      // Converts the bias voltage to a data bit - uses the 1100 to offset it with Vzero
      lpdac_cfg.DacData6Bit = 0x40 >> 1;            /* Set Vzero to middle scale - sets Vzero to 1.1V */
      lpdac_cfg.DacData12Bit = (uint16_t)((biasVolt + 1100.0f)/DAC12BITVOLT_1LSB);
    }
    else
    {
      /* 
        Working decently well now.
      */
      lpdac_cfg.DacData6Bit = (uint32_t)((zeroVolt-200)/DAC6BITVOLT_1LSB);
      lpdac_cfg.DacData12Bit = (int32_t)((biasVolt)/DAC12BITVOLT_1LSB) + (lpdac_cfg.DacData6Bit * 64);
      if(lpdac_cfg.DacData12Bit < lpdac_cfg.DacData6Bit * 64) lpdac_cfg.DacData12Bit--; // compensation as per datasheet 
    } 
    lpdac_cfg.DataRst = bFALSE;      /* Do not reset data register */
    // Allows for measuring of Vbias and Vzero voltages and connects them to LTIA, LPPA, and HSTIA
    lpdac_cfg.LpDacSW = LPDACSW_VBIAS2LPPA|LPDACSW_VBIAS2PIN|LPDACSW_VZERO2LPTIA|LPDACSW_VZERO2PIN|LPDACSW_VZERO2HSTIA;
    AD5940_LPDACCfgS(&lpdac_cfg);
    Serial.println("LPDAC configured successfully.");
  }

  /* Sets the input of the ADC to the output of the HSTIA */
  dsp_cfg.ADCBaseCfg.ADCMuxN = ADCMUXN_HSTIA_N;
  dsp_cfg.ADCBaseCfg.ADCMuxP = ADCMUXP_HSTIA_P;

  /* Programmable gain array for the ADC */
  dsp_cfg.ADCBaseCfg.ADCPga = ADCPGA_1;
  
  /* Disables digital comparator functionality */
  memset(&dsp_cfg.ADCDigCompCfg, 0, sizeof(dsp_cfg.ADCDigCompCfg));
  
  /* Is this actually being used? */
  dsp_cfg.ADCFilterCfg.ADCAvgNum = ADCAVGNUM_16; // Impedance example uses 16 
  dsp_cfg.ADCFilterCfg.ADCRate = ADCRATE_800KHZ;	/* Tell filter block clock rate of ADC*/
  dsp_cfg.ADCFilterCfg.ADCSinc2Osr = ADCSINC2OSR_1333; // Oversampling ratio for SINC2
  dsp_cfg.ADCFilterCfg.ADCSinc3Osr = ADCSINC3OSR_4; // Oversampling ratio for SINC3
  
  /* Bypassing filters so SINC settings don't matter */
  dsp_cfg.ADCFilterCfg.BpNotch = bTRUE; // Bypasses Notch filter
  dsp_cfg.ADCFilterCfg.BpSinc3 = bFALSE; // Bypass SINC3
  dsp_cfg.ADCFilterCfg.Sinc2NotchEnable = bTRUE; // Enables SINC2 filter

  dsp_cfg.DftCfg.DftNum = DFTNUM_16384; // Max number of DFT points
  dsp_cfg.DftCfg.DftSrc = DFTSRC_SINC3; // Sets DFT source to SINC3
  dsp_cfg.DftCfg.HanWinEn = bTRUE;  // Enables HANNING WINDOW - recommended to always be on 
  
  /* Disables STAT block */
  memset(&dsp_cfg.StatCfg, 0, sizeof(dsp_cfg.StatCfg));
  
  AD5940_DSPCfgS(&dsp_cfg); // Sets the DFT 
  Serial.println("DSP configured successfully.");

  /* Calculating Clock Cycles to wait given DFT settings */
  clks_cal.DataType = DATATYPE_DFT;
  clks_cal.DftSrc = DFTSRC_SINC3; // Source of DFT
  clks_cal.DataCount = 1L<<(DFTNUM_16384+2); /* 2^(DFTNUMBER+2) */
  clks_cal.ADCSinc2Osr = ADCSINC2OSR_1333;
  clks_cal.ADCSinc3Osr = ADCSINC3OSR_4;
  clks_cal.ADCAvgNum = ADCAVGNUM_16;
  clks_cal.RatioSys2AdcClk = sysClkFreq / adcClkFreq; // Same ADC / SYSTEM CLCK FREQ
  AD5940_ClksCalculate(&clks_cal, &_waitClcks);

  /* Clears any interrupts just in case */
  AD5940_ClrMCUIntFlag();
  AD5940_INTCClrFlag(AFEINTSRC_SINC2RDY);

  /* Do I need to include AFECTRL_HPREFPWR? It's the only one not here. */

   // Added bias option conditionally 
  if((biasVolt == 0.0f) && (zeroVolt == 0.0f))
  {
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                  AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                  AFECTRL_SINC2NOTCH, bTRUE);
    Serial.println("No bias applied.");
  }
  else
  {
    /* 
      Also powers the DC offset buffers that's used with LPDAC (Vbias) 
      Buffers need to be powered up here but aren't turned off in measurement like 
      the rest. This is to ensure the LPDAC and bias stays on the entire time.
    */
    AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                  AFECTRL_WG|AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                  AFECTRL_SINC2NOTCH|AFECTRL_DCBUFPWR, bTRUE);
    Serial.println("Bias is applied.");
  }

  AD5940_SleepKeyCtrlS(SLPKEY_LOCK); // Disables Sleep Mode 
  /* 
    Turns off waveform generator to only get the bias without excitation. 
  */
  AD5940_AFECtrlS(AFECTRL_WG, bFALSE);
  Serial.println("Everything turned on. WG is set off.");
  printf("Bias: %f, Zero: %f\n", biasVolt, zeroVolt);
}

void HELPStat::AD5940_ADCMeasure(void) {

  SWMatrixCfg_Type sw_cfg;
  uint32_t adcCode;
  uint32_t gainPGA = ADCPGA_1;
  float vRef1p82 = 1.82;

  /* Measuring ADC */
  sw_cfg.Dswitch = SWD_CE0; // CE0 disconnected from Output
  sw_cfg.Pswitch = SWP_RE0;
  sw_cfg.Nswitch = SWN_SE0;
  sw_cfg.Tswitch = SWT_SE0LOAD|SWT_TRTIA;

  AD5940_SWMatrixCfgS(&sw_cfg);

	AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|AFECTRL_SINC2NOTCH, bTRUE);

  // AD5940_AFECtrlS(AFECTRL_ADCPWR, bTRUE);  /* Enable ADC */
  AD5940_ADCPowerCtrlS(bTRUE);
  AD5940_Delay10us(16 * 25);
  
  // AD5940_AFECtrlS(AFECTRL_ADCCNV, bTRUE);  /* Start ADC convert and DFT */
  AD5940_ADCConvtCtrlS(bTRUE);
  AD5940_Delay10us(_waitClcks); // Empirical delay

  Serial.println("Polling ADC!");
  float vOut = pollADC(gainPGA, vRef1p82);

  // AD5940_AFECtrlS(AFECTRL_ADCPWR|AFECTRL_ADCCNV, bFALSE);  /* Stop ADC convert */
  AD5940_ADCPowerCtrlS(bFALSE);
  AD5940_ADCConvtCtrlS(bFALSE);

  printf("ADC Code: %d, ADC Voltage: %.4f\n", adcCode, vOut);
  AD5940_ShutDownS();
}

void HELPStat::ADCsweep(void)
{
  Serial.println("Running sweep!");
  for(int i = 0; i < 10; i++)
  {
    printf("Index: %d\n", i);
    AD5940_ADCMeasure();
    delay(2000);
  }
  AD5940_ShutDownS();
  Serial.println("Shutting down now.");
}

void HELPStat::PGACal(void)
{
  AD5940Err err;
  ADCPGACal_Type pgacal;
  pgacal.AdcClkFreq = 16e6;
  pgacal.ADCSinc2Osr = ADCSINC2OSR_178;
  pgacal.ADCSinc3Osr = ADCSINC3OSR_4;
  pgacal.SysClkFreq = 16e6;
  pgacal.TimeOut10us = 1000;
  pgacal.VRef1p11 = 1.11f;
  pgacal.VRef1p82 = 1.82f;
  pgacal.PGACalType = PGACALTYPE_OFFSETGAIN;
  pgacal.ADCPga = ADCPGA_1;
  err = AD5940_ADCPGACal(&pgacal);
  if(err != AD5940ERR_OK){
    printf("AD5940 PGA calibration failed.");
  }
}

/*
  Taken from Analog Devices ADC Polling Example and expounded on for noise
  measurements. Link: https://github.com/analogdevicesinc/ad5940-examples/blob/master/examples/AD5940_ADC/AD5940_ADCPolling.c
*/
void HELPStat::ADCNoiseTest(void)
{
  ADCBaseCfg_Type adc_base;
  ADCFilterCfg_Type adc_filter;
  SWMatrixCfg_Type sw_cfg;

  /* Constants for sampling rate at 120 Hz */
  uint32_t SAMPLESIZE = 7200; 
  uint32_t runTime = 60;
  float currentArr[SAMPLESIZE];
  float fSample = 120; 
  uint32_t delaySample = (1/fSample) * 1000; 
  
  /* Use hardware reset */
  AD5940_HWReset();

  /* Firstly call this function after reset to initialize AFE registers. */
  AD5940_Initialize();

  /* Configure AFE power mode and bandwidth */
  AD5940_AFEPwrBW(AFEPWR_LP, AFEBW_250KHZ);
  
  /* Testing with TDD Noise - Disabled Waveform Generator  */
  AD5940_TDDNoise(0.0, 0.0);

  /* Initialize ADC basic function */
  // AD5940_AFECtrlS(AFECTRL_DACREFPWR|AFECTRL_HSDACPWR, bTRUE); //We are going to measure DAC 1.82V reference.
  AD5940_AFECtrlS(AFECTRL_HSTIAPWR|AFECTRL_INAMPPWR|AFECTRL_EXTBUFPWR|\
                  AFECTRL_DACREFPWR|AFECTRL_HSDACPWR|\
                  AFECTRL_SINC2NOTCH|AFECTRL_DCBUFPWR, bTRUE);
  // Switches primarily control excitation amplifier. 
  // Only T switch really controls what gets into the HSTIA. 
  
  /* 
    As per recommendations from AD Tech Engineer, CE0 set to OPEN 
    If bias and vzero are enabled, set to SWD_CE0.
  */

  sw_cfg.Dswitch = SWD_OPEN;
  sw_cfg.Pswitch = SWP_RE0;
  sw_cfg.Nswitch = SWN_SE0;
  sw_cfg.Tswitch = SWT_SE0LOAD|SWT_TRTIA;
  AD5940_SWMatrixCfgS(&sw_cfg);

  /* Set HSTIA to use for measurement of SE0 - change this for each HSTIA */
  // HSTIARTIA_200
  // HSTIARTIA_1K
  // HSTIARTIA_5K
  // HSTIARTIA_10K
  // HSTIARTIA_20K
  // HSTIARTIA_40K
  // HSTIARTIA_80K
  // HSTIARTIA_160K

  AD5940_HSRTIACfgS(HSTIARTIA_1K);
  
  adc_base.ADCMuxP = ADCMUXP_HSTIA_P;
  adc_base.ADCMuxN = ADCMUXN_HSTIA_N;
  adc_base.ADCPga = ADCPGA_1;
  AD5940_ADCBaseCfgS(&adc_base);
 
  /* Initialize ADC filters ADCRawData-->SINC3-->SINC2+NOTCH */
  adc_filter.ADCSinc3Osr = ADCSINC3OSR_4;
  adc_filter.ADCSinc2Osr = ADCSINC2OSR_1333;
  adc_filter.ADCAvgNum = ADCAVGNUM_2;         /* Don't care about it. Average function is only used for DFT */
  adc_filter.ADCRate = ADCRATE_800KHZ;        /* If ADC clock is 32MHz, then set it to ADCRATE_1P6MHZ. Default is 16MHz, use ADCRATE_800KHZ. */
  adc_filter.BpNotch = bTRUE;                 /* SINC2+Notch is one block, when bypass notch filter, we can get fresh data from SINC2 filter. */
  adc_filter.BpSinc3 = bTRUE;                /* SINC3 filter is bypassed to read raw ADC results */ 
  adc_filter.Sinc2NotchEnable = bTRUE;        /* Enable the SINC2+Notch block. You can also use function AD5940_AFECtrlS */ 
  AD5940_ADCFilterCfgS(&adc_filter);
  
  //AD5940_ADCMuxCfgS(ADCMUXP_AIN2, ADCMUXN_VSET1P1);   /* Optionally, you can change ADC MUX with this function */

  /* Enable all interrupt at Interrupt Controller 1. So we can check the interrupt flag */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE); 

  AD5940_ADCPowerCtrlS(bFALSE);
  AD5940_ADCConvtCtrlS(bFALSE);
  
  printf("Index, Interval, ADC Code, SE0, RE0, SE0-RE0, (-SE0) - RE0\n");
  unsigned long totalTimeStart = millis();
  unsigned long timeStart = millis(); 
  uint32_t i = 0; 
  while(i < SAMPLESIZE) 
  {
    uint32_t rd;
    uint32_t rd_re0;
    unsigned long currTime = millis(); 

    if(currTime - timeStart >= delaySample)
    {
      unsigned long timeInt = currTime - timeStart; 
      timeStart = millis();
      adc_base.ADCMuxP = ADCMUXP_HSTIA_P;
      adc_base.ADCMuxN = ADCMUXN_HSTIA_N;
      adc_base.ADCPga = ADCPGA_1;
      AD5940_ADCBaseCfgS(&adc_base);
      AD5940_ADCPowerCtrlS(bTRUE);
      AD5940_ADCConvtCtrlS(bTRUE);
      delay(2);
      while(!AD5940_INTCTestFlag(AFEINTC_1,AFEINTSRC_ADCRDY)){}
      
      if(AD5940_INTCTestFlag(AFEINTC_1,AFEINTSRC_ADCRDY))  
      {
        
        AD5940_INTCClrFlag(AFEINTSRC_ADCRDY);
        rd = AD5940_ReadAfeResult(AFERESULT_SINC3);
      }
      
      AD5940_ADCPowerCtrlS(bFALSE);
      AD5940_ADCConvtCtrlS(bFALSE);

      /* Uncomment if Vzero and Vbias are enabled */
      // adc_base.ADCMuxP = ADCMUXP_VRE0;
      // adc_base.ADCMuxN = ADCMUXN_VREF1P1;
      // adc_base.ADCPga = ADCPGA_1;
      // AD5940_ADCBaseCfgS(&adc_base);
      // AD5940_ADCPowerCtrlS(bTRUE);
      // AD5940_ADCConvtCtrlS(bTRUE);
      // delay(1);

      // while(!AD5940_INTCTestFlag(AFEINTC_1,AFEINTSRC_ADCRDY)){}

      // if(AD5940_INTCTestFlag(AFEINTC_1,AFEINTSRC_ADCRDY))  
      // {
      //   AD5940_INTCClrFlag(AFEINTSRC_ADCRDY);
      //   rd_re0 = AD5940_ReadAfeResult(AFERESULT_SINC3);
      // }

      // AD5940_ADCPowerCtrlS(bFALSE);
      // AD5940_ADCConvtCtrlS(bFALSE);

      float diff_volt = AD5940_ADCCode2Volt(rd, ADCPGA_1, 1.816);
      // float re0_volt = AD5940_ADCCode2Volt(rd_re0, ADCPGA_1, 1.816); // Uncomment if using Vzero and Vbias
      float re0_volt = 0; // Comment if using Vzero and Vbias  
      
      /* 
        Optional Save Data for it - uncomment here
      */

      _noiseArr[i].idx = i;
      _noiseArr[i].interval = timeInt;
      _noiseArr[i].vSE0 = diff_volt; 
      _noiseArr[i].vRE0 = re0_volt;
      _noiseArr[i].diff = diff_volt - re0_volt;
      _noiseArr[i].diffInv = (-diff_volt) - re0_volt; // Inverse of diff given that VSE0 post TIA is (-)

      printf("%d, %lu, %d, %.6f, %.6f, %.6f, %.6f\n", i, timeInt, rd, diff_volt, re0_volt, diff_volt - re0_volt, (-diff_volt) - re0_volt);
      i++;
    }
  }
  unsigned long totalTimeEnd = millis();
  printf("Total time of experiment: %lu\n", totalTimeEnd - totalTimeStart);
  AD5940_ShutDownS();
}

void HELPStat::saveDataNoise(String dirName, String fileName)
{
  String directory = "/" + dirName;
  
  if(!SD.begin(CS_SD))
  {
    Serial.println("Card mount failed.");
    return;
  }

  if(!SD.exists(directory))
  {
    /* Creating a directory to store the data */
    if(SD.mkdir(directory)) Serial.println("Directory made successfully.");
    else
    {
      Serial.println("Couldn't make a directory or it already exists.");
      return;
    }
  }

  else Serial.println("Directory already exists.");

  /* Writing to the files */
  Serial.println("Saving to folder now.");

  // All cycles 
  String filePath = directory + "/" + fileName + ".csv";

  File dataFile = SD.open(filePath, FILE_WRITE); 
  if(dataFile)
  {
    dataFile.print("Index, ADC Code, VSE0, VRE0, VSE0-VRE0");
    for(uint32_t i = 0; i < NOISE_ARRAY - 1; i++)
    {
      dataFile.print(_noiseArr[i].idx);
      dataFile.print(",");
      dataFile.print(_noiseArr[i].interval);
      dataFile.print(",");
      dataFile.print(_noiseArr[i].vSE0);
      dataFile.print(",");
      dataFile.print(_noiseArr[i].vRE0);
      dataFile.print(",");
      dataFile.print(_noiseArr[i].diff);
      dataFile.print(",");
      dataFile.print(_noiseArr[i].diffInv);
    }

    dataFile.close();
    Serial.println("Data appended successfully.");
  }
}

void HELPStat::AD5940_HSTIARcal(int rHSTIA, float rcalVal)
{
  HSRTIACal_Type rcalTest; // Rcal under test
  FreqParams_Type freqParams;
  fImpPol_Type polarResults; 
  AD5940Err testPoint; 

  rcalTest.fFreq = 120.0; // 120 Hz sampling frequency so we test at 120 Hz
  rcalTest.fRcal = rcalVal; 
  rcalTest.SysClkFreq = 16000000.0; // 16 MHz ADC and System Clck 
  rcalTest.AdcClkFreq = 16000000.0;

  /* Configuring HSTIA */
  rcalTest.HsTiaCfg.DiodeClose = bFALSE;
  rcalTest.HsTiaCfg.HstiaBias = HSTIABIAS_1P1; 
  rcalTest.HsTiaCfg.HstiaCtia = 31; 
  rcalTest.HsTiaCfg.HstiaDeRload = HSTIADERLOAD_OPEN; 
  rcalTest.HsTiaCfg.HstiaDeRtia = HSTIADERTIA_OPEN; 
  rcalTest.HsTiaCfg.HstiaRtiaSel = rHSTIA; 

  freqParams = AD5940_GetFreqParameters(120.0); 

  /* Configuring ADC Filters and DFT */
  rcalTest.ADCSinc2Osr = freqParams.ADCSinc2Osr;
  rcalTest.ADCSinc3Osr = freqParams.ADCSinc3Osr;
  rcalTest.DftCfg.DftNum = freqParams.DftNum;
  rcalTest.DftCfg.DftSrc = freqParams.DftSrc;
  rcalTest.DftCfg.HanWinEn = bTRUE; 

  rcalTest.bPolarResult = bTRUE; 

  testPoint = AD5940_HSRtiaCal(&rcalTest, &polarResults);

  if(testPoint != AD5940ERR_OK) Serial.println("Error!");

  printf("RTIA resistor value: %f\n", polarResults.Magnitude);

  AD5940_ShutDownS();
}
