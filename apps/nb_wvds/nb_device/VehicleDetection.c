/**************************************************************************************************
Filename:       VehicleDetection.c
Created:        $Date: 2011-11-23 20:34:23 $
Revised:        $Date: 2013-09-06 11:30:23 $
Revision:       $Revision: 101 $

Description:    This file contains the Vehicle Detection algorithm.
**************************************************************************************************/

/*********************************************************************
* INCLUDES
*/
#include "stdlib.h"
//#include "main_conf.h"
#include "VehicleDetection.h"
//#include "lib/random.h"
//#include "lib/crc16.h"
#include "app.h"
#include "stdlib.h"
//#include "crc16.h"

/*********************************************************************
* GLOBAL VARIABLES
*/ 
 extern struct Sample_Struct One_Sample; // xyz data form app ...
 extern struct ALGO algo; // algorithm data form app ...



/*********************************************************************
* LOCAL VARIABLES
*/
//the coordinator of x, y, z
cardet_axis_t XYZ[3];
compositive_t signal;
uint8_t enter_branch_num=0;
uint16_t max_min_value=0;//�����ļ���Сֵ
uint16_t current_thresh=0;
uint16_t need_parking_count=0;
uint8_t current_single=SINGLE_THRSH;
uint8_t current_double=DOUBLE_THRSH;
uint8_t current_third=THIRD_THRSH;
uint8_t bs_lock_count=0;
uint8_t bs_init_wait_count=0;
uint8_t temp_status=0;
uint8_t a_over_count=0;
uint8_t a_below_count=0;
uint8_t re_value=0;
bool bs_lock_flag=false;
bool is_work_flag=true;
bool bs_flag=false;//the update flag of baseline
bool init_base_line_flag=false;
int16_t dist1[3]={0,0,0};
int16_t dist2[3]={0,0,0};
uint8_t dist_count=0;
uint16_t sampleT = 0; //...
uint8_t enter_leave_branch=0; //��ǳ���ʻ���ʻ��ķ�֧��������...

//extern uint8_t Get_Gain_HMC5983(void);
//extern void Set_Gain_HMC5983(uint8_t gain);


#if (defined BS_DEBUG) && (BS_DEBUG == true)
flag_value_t flags_value;
bool debug_change_flag = false;
bool debug_branch_flag = false;


#endif
#if DEBUG_OUT==1
extern FILE *outfile;
extern FILE *outwaveinfo;
extern char* sdate[24];
void WriteFile(void);
int16_t sample_period=0;
void WriteWaveInfo(uint8_t wave);
void Debug_Adaptive_BS(void);
#endif


#if (defined BS_DEBUG) && (BS_DEBUG == true)
static void _debug(uint8_t field);
void Debug_Init(void);
#endif

#if UP_HILLS==1
upload_wave wave;
static uint8_t up_delay_count=0;
static uint8_t up_hill_changed = 0;
uint8_t up_point=0; // the head point of hill_valley[]
int16_t temp_hill[DIV_NUM][DIV_NUM]={0};
void Set_Upload_Data(uint8_t branch, uint8_t index);
void Init_Upload_Struct(void);
void delay_call_up(void);
#endif

void (*activated_cb)(void);


// reset sample period
void Set_AMR_Period(int period)
{
  if (period < 16) {
    return;
  }
//  sampleT = ((uint32_t)period * CLOCK_SECOND) / 1000; // ���ڱ��β�����ɺ���Ч
  sampleT = period; // ���ڱ��β�����ɺ���Ч
}

// get sample period
uint16_t Get_AMR_Period() // app need call this func to set sample frequect
{
  return sampleT;
  
}

 // re init algorithm ...
void Re_Init_Request()
{
  signal.re_initial_flag = true;
  signal.initialize_flag = 0;
  signal.wait_flag = false;
}

//void Save_Algorithm_Parameters(void)
//{
#if WRITE_ALGO_FLASH==1
//  Get_Algorithm_Parameters(&algo_para); //Ϊstruct ALGO algo��ֵ
 // algo_para.crc = crc16_data((uint8_t*)&algo_para, sizeof(struct ALGO)-2, CRC_INIT);
  //nv_write(NV_ALGO_ADDR, (uint8_t*)&algo_para, sizeof(struct ALGO));//algo��ֵдflash
//#endif
//}
//#if IS_SNIFFER_VERSION==0
/*********************************************************************
* @fn      Set_Algorithm_Parameters
*
* @brief   Setting the Algorithm_Parameters by command dissemination
*
* @param   paras: the parameters of algorithm
*          is_from_pc=true �Ǵ���λ���·���ָ��������Ϣ����Ҫ���浽flash
*          is_from_pc=false ����֮���flash����ò��������ǳ�ʼ����Ĭ��ֵ������Ҫдflash
*
* @return  none
*/
int Set_Algorithm_Parameters(struct ALGO * paras)
{
  uint8_t i=0;
  //occ_thresh must bigger than unocc_thresh
  if(paras->big_occ_thresh < paras->unocc_thresh)
    return 1;

  //range limit
  //if(paras->big_occ_thresh > 50 && paras->big_occ_thresh < 200)
  {
    signal.occ_big_threshold = paras->big_occ_thresh;
    signal.occ_mid_threshold = paras->mid_occ_thresh;
    signal.occ_little_threshold = paras->litt_occ_thresh;
    signal.axis_stable_threshold = paras->axis_stable_threshold; // ...
  }
  //if(paras->unocc_thresh > 15 && paras->unocc_thresh < 100)
   signal.unocc_threshold = paras->unocc_thresh;

  signal.stable_sample_freq = paras->normalT;
  signal.disturb_sample_freq = paras->flunctT;
 /// Set_Gain_HMC5983(paras->gain_hmc5983);

  //set value for baseline 
  for(i=0;i<3;i++)
  {
    XYZ[i].adaptive_base_line = paras->base_line[i];
    XYZ[i].setup_base_line = paras->base_line[i];
    XYZ[i].base_line = paras->base_line[i];
  }

  signal.status=paras->status;

  //ͨ����λ�����ò������²���дflash
 // if(is_from_pc)
  //{
  //  Save_Algorithm_Parameters();
 // }

  return 0;
}


/*********************************************************************
* @fn      Get_Algorithm_Parameters
*
* @brief   Get the Algorithm_Parameters by command dissemination
*
* @param   none
*
* @return  none
*/

void Get_Algorithm_Parameters(struct ALGO * paras)
{
  uint8_t i=0;
  paras->magic = NV_MAGIC;
  paras->big_occ_thresh = signal.occ_big_threshold;
  paras->mid_occ_thresh = signal.occ_mid_threshold;
  paras->litt_occ_thresh =signal.occ_little_threshold;
  paras->unocc_thresh = signal.unocc_threshold;
  paras->flunctT = signal.disturb_sample_freq;
  paras->normalT = signal.stable_sample_freq;
  paras->axis_stable_threshold = signal.axis_stable_threshold;
 // paras->gain_hmc5983 = Get_Gain_HMC5983();
  //set value for baseline
  for(i=0;i<3;i++)
  {
    paras->base_line[i] = XYZ[i].adaptive_base_line;
  }
  paras->status = signal.status;
}
#endif




/*********************************************************************
* GLOBAL FUNCTIONS
*/

/***
*��ֵ�������ȵ�У��
*   �ٶ����趨����ֵTΪ������g=390(gain)�µ�ֵ
*   ����������g1�µ���ֵΪT1=T*g1/g
*   ���¸���5%����T1*=95/100���Ȳ�д����ɣ�
****/
void Thershold_Init_By_Gain()
{
  uint8_t j, k, z;
  smooth_buf_t* smooth;
  for(k=0; k<3; k++)
  {
    XYZ[k].axis_stable=true;
    XYZ[k].raw_value = 0;
    XYZ[k].base_line = 0;
    XYZ[k].smooth_value = 0;
    XYZ[k].delay_smooth = 0;
    XYZ[k].short_smooth = 0;
    XYZ[k].hist_short_smooth = 0;
    XYZ[k].hist_raw_value = 0;
    XYZ[k].setup_base_line = 0;
    XYZ[k].stable_thresh = STABLE_TH;  // 25
    //XYZ[k].unoccupied_thresh = UNOCCUPIED_TH;
    XYZ[k].long_stable_value=0;

    XYZ[k].hist_smooth_value = 0;
    XYZ[k].max_min_amplitute= 0;
    for(j=0; j<CHANGE_NUM; j++)
    {
      XYZ[k].changes[j] = 0;
    }

    smooth = &XYZ[k].smooth;

    smooth->head=0;
    smooth->hist=0;
    smooth->len=0;
    smooth->short_sum=0;
    smooth->sum=0;
    smooth->tail=0;
    for(z=0;z<SMOOTH_LENGTH;z++)
    {
      smooth->samples[z] = 0;
    }

  }
  signal.hist_is_strong_mag=false;
  signal.adaptive_bs_wait_num=0;
  signal.status=0;
  signal.st_before_fluct=0;
  signal.stable=true;
  signal.latest_stable=true;
  signal.continue_sample_num=0;
  signal.new_change_flag=false;
  signal.initialize_flag=0;
  signal.num_count=0;
  signal.rf_num_count=0;
  signal.init_num_count=0;
  signal.weak_num_count = 0; // ...
  signal.strong_num_count=0;
  signal.is_strong_mag = false; //...
  signal.re_initial_flag=false;
  signal.activate_rf=0;
  signal.compute_long_bs=false;
  signal.fuse_status=0;
  signal.h_below_count=0;
  signal.over_count=0;
  signal.p_over_count=0;
  signal.below_count=0;
  signal.b_status = 0;
  signal.unfluct_parking=false;
  signal.p_status = 0;
  signal.thunder_count_flag=false;
  signal.thunder_count=0;
  signal.is_thunder[0] = false;
  signal.is_thunder[1] = false;

}

/*********************************************************************
* @fn      Re_Init_Algorithm
*
* @brief   ���±궨,�ô������ٴν����ʼ��
*
* @param   none
*
* @return  none
*/
void Re_Init_Algorithm()
{
  //if(!signal.re_initial_flag)
  //{
    Thershold_Init_By_Gain();
 //   signal.re_initial_flag=false;//signal.re_initial_flag=true;
 //   signal.wait_flag=false;
    Set_AMR_Period(1120);  //������������Ϊ1��
  //}
}


/*********************************************************************
* @fn      Variant_Init
*
* @brief   Init the variants
*
* @param   none
*
* @return  none
*/
void Variant_Init()
{
  Thershold_Init_By_Gain();
  signal.wait_flag=false;
  signal.invoke_flag=true; // change to true for test ...
   
  {
    signal.stable_sample_freq = STABLE_SAMPLE_PERIOD; //CLOCK_SECOND // 560
    signal.disturb_sample_freq = DISTURB_SAMPLE_PERIOD; //CLOCK_SECOND // 96
    signal.occ_little_threshold = PARKING_THRESHOLD_L; //75
    signal.occ_mid_threshold = PARKING_THRESHOLD_B; //120
    signal.occ_big_threshold = PARKING_THRESHOLD_F; //180
    signal.unocc_threshold = UNOCC_THRSHOLD; //50
    signal.axis_stable_threshold = STABLE_TH; //25 ...
  }
  sampleT = INIT_SAMPLE_PERIOD; // 1120...

#if UP_HILLS==1
  Init_Upload_Struct();
#endif
}




/*********************************************************************
* @fn      Parking_Algorithm
*
* @brief   This function processes HMC magnetic field, it is the main function
*          of parking algorithm, include four steps
*          1) smooth the raw signal;
*          2) compute the adaptive base line;
*          3) compute the status of the parking space;
*          4) confirm the change of status by a simple state machine.
*
* @param   *buf - point to the buffer of magnegic values
*          x_scale, y_scale, z_scale  - the scale for temperature compensation
*
* @return  the status flag of the parking space
*/
uint8_t Parking_Algorithm(void)
{
  uint8_t re_value = 3;
  signal.b_status = 0;
  
 

  //the VD is invoked by detecting the constant magnetic signal five times in succession
  //��״̬��2����Ϊ3��ʾ�������
//#if IS_SNIFFER_VERSION==0  //0
/*  if(!signal.invoke_flag)
  {
    check_constant_signal(1);
    if(is_work_flag==true && signal.re_initial_flag==false)
    {
      is_work_flag=false;
      Set_AMR_Period(1120);  //������������Ϊ1��
    //   Set_AMR_Period(2240);  //������������Ϊ1��
    }
    return 2; //�ȴ�����
  }
//#endif

  if(is_work_flag==false)  
  {
    if(signal.re_initial_flag==true)
    {
      is_work_flag=true;
    }
    else
    {
      return 2; // after
    }
  }
*/


 
  //������ǰINIT_DEPLOYMENT_NUM������������Ϊ���ڴų��Ŷ��Ŀ����Դ�
  //��һ��mmc3316_reset,Ϊ��ʼ����׼��
  //signal.initialize_flag==0 ����ڵ�����֮���ٴν����ʼ��
  //��������signal.initialize_flag==2
  if((!signal.wait_flag && signal.initialize_flag == 0) || (!signal.wait_flag && signal.re_initial_flag == true)) // at init process clear initial_flag
  {
    //ǿ�ż������Ҫreset������
    re_value = wait_and_reset();
   // return 3;
    return re_value; // 4 �ȴ��ų���λ(��Ҫ�ƿ�ǿ��)  3, drop first 10 sample befor init
  }
  //set raw value
  //if ( nb module if is idle)
  //{
 // check_constant_signal(2);// to check the qiangci, if it is qiangci mean that it is a activate operation
  //}
  
  
    // big mag check  
  //check_constant_signal(1);
  if(check_constant_signal(1))
  { 
    return 2; // big mag 
  }
  
  Set_Raw_Value();

  //Reset_After_Strong_Mag();

//#if ACTIVATE_RF_BY_AMR==1
 // if(signal.activate_rf==2 || signal.activate_rf==1) //����RF �� ���ų���λ
// {
    //Set_AMR_Period(1120);  //������������Ϊ1��
    //return (signal.activate_rf+1);
  //  return signal.status; //return re_value;
 // }
//#endif

  //data filter with average smooth
  Raw_Data_Smooth();

  //�����10~30�����ǳ�ʼ���׶Σ��ڵ㱣�ֲ����������ܵ��Ÿ���
  if(signal.initialize_flag == 0)
  {
     // if(signal.re_initial_flag == true && signal.wait_flag == false)
    if(signal.re_initial_flag == true )
    {
      signal.re_initial_flag = false;
      Re_Init_Algorithm(); // re init handle 
      Get_Algorithm_Parameters(&algo); // get algo parmeters ...
    }
    Init_process();
    re_value=3; // 4 represented the initialize phase
  }
//#if WRITE_ALGO_FLASH==1
 // else if(signal.initialize_flag==2) //����,�����ָ�
 // {
 //   nv_read(NV_ALGO_ADDR, (uint8_t*)&algo_para, sizeof(struct ALGO));//��flash
//    Set_Algorithm_Parameters(&algo_para,false);//����flash�е�ֵ���г�ʼ��
//    signal.initialize_flag=1;
 //   re_value=signal.status;
//  }
//#endif
  else
  {
    //check each of x, y, z is un-stable
    //XYZ_IS_Fluctuation();
    //check each of x, y, z is return to stable state
    XYZ_IS_Stable();
    //check the signal's stable state and
    Signal_IS_Stable();
    //setting the hill and valley of a continuous fluctucation
    Statistic_Wave_Shape();
    //adaptive sample mechanism
    Adaptive_Sampling();

    //adaptive update base line
    if(signal.adaptive_bs_wait_num>0)  // set at IS_leaving, 100
    {
      Adaptive_base_line();
    }

    //�����ΪʲôҪ�ȶ�ʱ���жϣ�����ܵ����ţ��ȶ���������
    //if(signal.status != 1 && signal.stable)
    if(signal.status != 1 )
    {
      IS_Parking();
    }
    //else if(signal.is_thunder[1] && signal.status==1) //signal.is_thunder[1]���˳�ʱ�����޷�����
    //�˳�ʱҲ�����ȶ������������
    else if(signal.status!=0 && signal.stable)
    {
      IS_Leaving();
    }
    //Compute_parking_drift();//�г�״̬�£�����200�β������޲����ģ�delay_smooth - base_line
    Record_Long_Stable();

    //���źŲ���ʱ������base_line
    //�ڲ���ȷ���޳�״̬�²�����base_line
    if(signal.b_status==0 && signal.status==0 && signal.stable && !bs_lock_flag) // no park,after signal stable and 50 times later
    {
      Base_Line_Smooth();
    }

#if (defined BS_DEBUG) && (BS_DEBUG == true)
    if(!debug_change_flag && !debug_branch_flag)
    {
      _debug(BASE_LINE);
      _debug(SMOOTH_VALUE);
    }
#endif

    // for quick arrival response
    //if(signal.status==0)
    //{
    //  Fast_Arrival_Response(); //�ڵ���·�β���ʹ�øú�������Ϊ���ܻ����0,1��������
    //}

    // for quick leaving response
    if(signal.status==1)
    {
      Fast_Leaving_Response();
    }

   re_value=signal.status;

#if UP_HILLS==1
    //�����������ӳ��ϴ�����ֵ
    delay_call_up();
#endif
  }
#if DEBUG_OUT==1
  WriteOutFile(re_value, " parking= ", outfile);
  WriteFile();
  fputs("\r\n", outfile);
#endif
  Set_His_Value();
  return re_value;
}



/*********************************************************************
* LOCAL FUNCTIONS
*/


/*********************************************************************
* @fn      Set_Hist_Value
*
* @brief   Set history value
*
*
* @param   none
*
* @return  none
*/
void Set_His_Value()
{
  uint8_t i=0;
  for(i=0; i<DIV_NUM; i++)
  {
    XYZ[i].hist_raw_value = XYZ[i].raw_value;
    XYZ[i].hist_short_smooth = XYZ[i].short_smooth;
  }
  if(signal.latest_stable != signal.stable)
  {
    signal.latest_stable=signal.stable;
  }
  signal.hist_is_strong_mag=Is_Strong_Magnetic();
}

/*********************************************************************
* @fn      Adaptive_base_line
*
* @brief   adaptive update base line
*
*
*
* @param   none
*
*
* @return  none
*/
void Adaptive_base_line()
{
  uint8_t i=0;
  if(signal.adaptive_bs_wait_num==1)
  {
    // update base line
    for(i=0;i<DIV_NUM;i++)
    {
      Fill_base_buf(&XYZ[i].bs_buf,XYZ[i].smooth_value);
      Update_base_line(&XYZ[i]);
    }
#if DEBUG_OUT==1
    Debug_Adaptive_BS();
#endif
  }
  if(signal.adaptive_bs_wait_num > 0)
  {
    signal.adaptive_bs_wait_num--;
  }
}

/*********************************************************************
* @fn      Fill_base_buf
*
* @brief   Fill smooth value in bs_buf
*
*
*
* @param   *bs_buf -  the pointer of one bs_buf
*          *current_value - the current smooth value
*
* @return  none
*/
void Fill_base_buf(bs_buf_t* bs_buf, int16_t current_value)
{
  uint8_t ins;

  ins = bs_buf->tail + 1;
  if(ins >= BS_BUFFER_LEN) //6
    ins = 0;
  if(ins == bs_buf->head)
  {
    bs_buf->sum -= bs_buf->samples[bs_buf->head]; // minus oldest sample from sum
    bs_buf->head += 1; // drop oldest sample
    if(bs_buf->head >= BS_BUFFER_LEN)
    {
      bs_buf->head = 0;
    }
  }
  bs_buf->samples[bs_buf->tail] = current_value;
  bs_buf->sum += current_value;
  bs_buf->tail = ins;

  if (bs_buf->tail > bs_buf->head)
    bs_buf->len = bs_buf->tail - bs_buf->head;
  else
    bs_buf->len = bs_buf->tail + BS_BUFFER_LEN - bs_buf->head;

  if(bs_buf->len>0)
  {
    bs_buf->average_value = bs_buf->sum / bs_buf->len;
  }
}


/*********************************************************************
* @fn      Update_base_line
*
* @brief   if all values in bs_buf allmost is equal, update base_line
*
*
*
* @param   *bs_buf -  the pointer of one bs_buf
*          *smooth - the pointer of smooth struct
*
* @return  none
*/
void Update_base_line(cardet_axis_t * axis) //bs_buf_t* bs_buf, smooth_buf_t* smooth)
{
  uint8_t i=0;
  bs_buf_t * bs_buf = & axis->bs_buf;
  if(bs_buf->len == (BS_BUFFER_LEN-1)) //��ʾbs_buf�Ѿ�����
  {
    i = bs_buf->head;
    while(i != bs_buf->tail)
    {
      if((bs_buf->samples[i] - bs_buf->average_value) > UPDATE_BS_T)
      {
        return;
      }
      else
      {
        i++;
        if(i>= BS_BUFFER_LEN)
        {
          i=0;
        }
      }
    }

    axis->adaptive_base_line = bs_buf->average_value; //���»��ߣ���Ҫ����д

 //   Save_Algorithm_Parameters();
  }
}

void Record_Long_Stable() // excute after status change to 1
{
  uint8_t i=0;
  if(signal.stable && signal.stable!=signal.latest_stable) //from unstable to stable
  {
    signal.long_stable_num = LONG_STABLE+20; // 100
    if(!signal.compute_long_bs && !base_enter_change_is_all_zero())
    {
      for(i=0;i<DIV_NUM;i++)
      {
        XYZ[i].changes[BASE_LEAVE] = XYZ[i].short_smooth - XYZ[i].long_stable_value;
      }
#if DEBUG_OUT==1
      //WriteWaveInfo(1);
#endif
    }
  }
  else if(signal.stable && signal.stable==signal.latest_stable && signal.status==1) // the signal is continuous stable after the car parking
  {
    if(signal.long_stable_num>0)
    {
      signal.long_stable_num--;
    }
    else
    {
      if(signal.compute_long_bs)
      {
        for(i=0;i<DIV_NUM;i++)
        {
          XYZ[i].changes[BASE_ENTER] = XYZ[i].smooth_value - XYZ[i].base_line;
        }
        signal.compute_long_bs = false;
      }

      for(i=0;i<DIV_NUM;i++)
      {
          XYZ[i].long_stable_value = XYZ[i].smooth_value;
      }
#if DEBUG_OUT==1
      //WriteWaveInfo(1);
#endif
      signal.long_stable_num = LONG_STABLE;
    }
  }
}

/*********************************************************************
* @fn      Fast_Arrival_Response
*
* @brief   fast response function according to [average_value - base_line]
*
* @param   none
*
* @return  none
*/
void Fast_Arrival_Response(void)
{
  // for quick response
  uint8_t a_status=0;

  a_status =  Smooth_Base_Status(PARKING_THRESHOLD_F); //Average_Over_Thresh(AVERAGE_THRESH);//ֻҪ����һ���ᳬ��AVERAGE_THRESH,����Ϊ�г�
  //a_status = Smooth_Base_Status(current_thresh,SINGLE_THRSH_1,  DOUBLE_THRSH_1);//�����֮�ͳ�����ֵ������Ϊ�г�
  if(a_status == 1)
  {
    if(temp_status == 0)
    {
      a_over_count++;
      if(a_over_count >= AVERAGE_OVER)
      {
        temp_status=1;
        Car_Arrival_Functions(AVERAGE_ARRIVAL);
        a_over_count=0;
#ifdef SIM
        //printf("enter %d, base=[%d,%d,%d], cO=%d\n", 3, base_line[0], base_line[1], base_line[2], a_over_count);
#endif
      }
    }
    a_below_count=0;
  }
}


/*********************************************************************
* @fn      Fast_Leaving_Response
*
* @brief   fast response function according to [average_value - base_line]
*
* @param   none
*
* @return  none
*/
void Fast_Leaving_Response(void)
{
  // for quick response
  uint8_t a_status=0,f_leaving_branch=0;
  a_status = Is_Below_Thresh(FAST_LEAVING_THRSH, SMOOTH_BS);
  if(a_status == 1 )
  {
    f_leaving_branch=7;
  }
  else
  {
    a_status = Is_Below_Thresh(FAST_LEAVING_THRSH, ADAPTIVE_BS);
    if(a_status==1)
    {
      f_leaving_branch=8;
    }
    else
    {
      a_status = Is_Below_Thresh(FAST_LEAVING_THRSH, SETUP_BS);
      if(a_status==1)
      {
        f_leaving_branch=9;
      }
    }
  }

  if(a_status == 1)
  {
     a_over_count++;
     if(a_over_count >= CAR_LEAVE_COUNT) //5
     {
       //Car_Leaved_Init();
       Car_Leave_Functions(f_leaving_branch);
       a_over_count=0;
     }

  }
  else
  {
     a_over_count=0;
  }
}

/*********************************************************************
* @fn      Smooth_Base_Status
*
* @brief   return the status of the parking slot
*
* @param   a_thresh - the first threshold
*          b_thresh - the second threshold
*
* @return  status
*/
uint8_t Smooth_Base_Status(uint16_t a_thresh)
{
  uint8_t a_status =0, i=0, temp_status=0;
  //a_status = Is_Over_Thresh(b_thresh, c_thresh, c_thresh);
  uint16_t t_value = 0;

  //if(a_thresh==signal.occ_big_threshold)

  for(i=0; i<3; i++)
  {
    t_value += abs(XYZ[i].short_smooth - XYZ[i].base_line);
    //t_value += abs(XYZ[i].smooth_value - XYZ[i].base_line);
  }
  temp_status = Is_Below_Thresh(FAST_LEAVING_THRSH, ADAPTIVE_BS); // 35  12
  if(t_value>=a_thresh && temp_status!=1)
  {
    a_status=1;
  }
  return a_status;
}

/*********************************************************************
* @fn      Debug_Init
*
* @brief   variants inition for debug
*
* @param   none
*
* @return  none
*/
#if (defined BS_DEBUG) && (BS_DEBUG == true)
void Debug_Init(void)
{
  debug_change_flag = false;
  debug_branch_flag = false;

  flags_value.data.thresMatch = 0;
  flags_value.data.reseted = 0;
  flags_value.data.car_change = 0;
  flags_value.data.new_change = 0;

  enter_leave_branch=0;

}
#endif

/*********************************************************************
* @fn      Reset_Base_Line
*
* @brief   reset base_line[] array
*
* @param   none
*
* @return  none
*/
/*
void Reset_Base_Line()
{
  uint8_t i=0;
  for(i=0; i<3; i++)
  {
    XYZ[i].base_line = XYZ[i].smooth_value;
    //XYZ[i].hist_base_line = 0;
  }
}
*/

/*********************************************************************
* @fn      Is_All_Zero
*
* @brief   check there is all zero values in the a_array[]
*
* @param   a_array
*
* @return  is all zero
*/
bool Is_All_Zero(int16_t a_array[])
{
  bool is_all=false;
  uint8_t i=0;
  for(i=0;i<3;i++)
  {
    if(a_array[i]!=0)
    {
      break;
    }
  }
  if(i>=3)
  {
    is_all = true;
  }
  return is_all;
}

/*********************************************************************
* @fn      Car_Arrival_Functions
*
* @brief   variants setting and debug message sending when car is detected
*
* @param   arrival_num - the enterence for car detection cases in algorithm
*
* @return  none
*/
void Car_Arrival_Functions(uint8_t arrival_num)
{
    //over_count = 0;
    signal.status = 1;
    enter_leave_branch = arrival_num;

 //   Save_Algorithm_Parameters();
    //watchdog_reboot(); //��������
#ifdef SIM
  printf("enter %d\n", arrival_num);
#endif
#if (defined BS_DEBUG) && (BS_DEBUG == true)

  debug_branch_flag = true;
  _debug(ENTER_BRANCH);
#endif
#if DEBUG_OUT==1
  fputs(sdate,outwaveinfo);
  WriteOutFile(arrival_num, " enter ", outwaveinfo);
  fputs("\r\n", outwaveinfo);
#endif
}

/*********************************************************************
* @fn      Car_Leave_Functions
*
* @brief   variants setting and debug message sending when car is leaved
*
* @param   leave_num - the enterence for car leave cases in algorithm
*
* @return  none
*/
void Car_Leave_Functions(uint8_t leave_num)
{
  Car_Leaved_Init();
  enter_leave_branch = leave_num;
  signal.adaptive_bs_wait_num=BS_WAIT_NUM; // 100
//  Save_Algorithm_Parameters();
  if(bs_flag)
  {
    bs_flag=false;
  }

  //Re_Init_Algorithm(); //test Re_Calibrate_AMR

#ifdef SIM
  printf("leave %d, cI=3\n", leave_num);
#endif
#if (defined BS_DEBUG) && (BS_DEBUG == true)

  debug_branch_flag = true;
  _debug(LEAVE_BRANCH);
#endif
#if DEBUG_OUT==1
  fputs(sdate,outwaveinfo);
  WriteOutFile(leave_num, " leave ", outwaveinfo);
  fputs("\r\n", outwaveinfo);
#endif
}


/*********************************************************************
* @fn      Car_Leaved_Init
*
* @brief   the car is leaved, and init variants
*
* @param   none
*
* @return  none
*/
void Car_Leaved_Init()
{
  uint8_t i=0,j=0;
  //s_status=0;
  signal.status=0;
  signal.b_status=0;
  signal.below_count=0;
  signal.over_count = 0;
  signal.is_thunder[0] = false;
  signal.is_thunder[1] = false;
  signal.new_change_flag = false;
  signal.unfluct_parking = false;
  signal.compute_long_bs=false;
  bs_flag=false;

  for(i=0;i<3;i++)
  {
    for(j=0;j<CHANGE_NUM;j++)
    {
      XYZ[i].changes[j]=0;
    }
    XYZ[i].hist_smooth_value=0;
    XYZ[i].long_stable_value=0;
  }
}

/*********************************************************************
* @fn      Is_Base_Car_Leaved
*
* @brief   decision car_is_leaved according to [smooth_value - baseline]
*
* @param   a_array[] - the value of [smooth_value - baseline] for car arrival
*          b_array[] - the value of [smooth_value - baseline] for car leaving
*
* @return  the matching degrees of a_array and b_array
*/
uint8_t Is_Base_Car_Leaved(uint8_t ca, uint8_t cb)
{
  uint8_t i=0, j=0;
  uint16_t change_rate=100;
  int16_t a_array[DIV_NUM];
  int16_t b_array[DIV_NUM];

  for(i=0;i<3;i++)
  {
    a_array[i] = XYZ[i].changes[ca];
    b_array[i] = XYZ[i].changes[cb];
  }

  for(i=0;i<3;i++)
  {
    if(abs(a_array[i])!=0 && abs(b_array[i]) <= 320)
    {
      change_rate = abs(b_array[i]) * 100 / abs(a_array[i]);
      if(change_rate <= CAR_CHANGE_RATE)
      {
        j++;
      }
    }
    else
    {
      change_rate = 100;
    }
  }

  return j;
}

uint8_t Is_Change_Sim(uint8_t ca, uint8_t cb)
{
  uint32_t change_value=0, change_dis=0;
  uint8_t i=0, sim=0;
  uint16_t change_rate=0;
  int16_t a_array[DIV_NUM];
  int16_t b_array[DIV_NUM];

  for (i=0; i<DIV_NUM; i++) {
    a_array[i] = XYZ[i].changes[ca];
    b_array[i] = XYZ[i].changes[cb];
  }

  for(i=0;i<3; i++)
  {
    change_value += abs(a_array[i] + b_array[i]);
    change_dis += (abs(a_array[i])>abs(b_array[i])) ? abs(a_array[i]) : abs(b_array[i]);
  }
  if(change_dis!=0)
  {
    change_rate = change_value * 100 / change_dis;
  }
#if DEBUG_OUT==1
  WriteOutFile(change_rate, " change_rate ", outwaveinfo);
  fputs("\r\n", outwaveinfo);
#endif
  if(change_rate <= CAR_CHANGE_RATE)
  {
    sim=1;
  }
  else if(change_dis>100 && change_rate <= CAR_CHANGE_RATE_2) //120 for HMC5983, 200 for MMC3316
  {
    sim=1;
  }

  if(change_dis>1200)//����MMC3316�������ǳ�����ǿ�Ÿ���
  {
    if(change_rate <= CAR_CHANGE_RATE_1)
    {
#if WITH_MMC3316
      MMC3316_Reset(SENSOR_PORT);
#elif WITH_HMC5983 || WITH_QMC5883
//#warning "no support"
#else
//#error "no support"
#endif
    }
  }

  return sim;
}

/*********************************************************************
* @fn      Is_Car_Leaved
*
* @brief   determine wheather the car is leaved according to the appropriate
*          changes of car_arrival and car_leave
*
* @param   none
*
* @return  is car leaved
*/
// �ж�a_array��b_array���������е�ֵ�Ƿ����
uint8_t Is_Car_Leaved(uint8_t ca, uint8_t cb)
{
  uint8_t i=0,j=0,k=0;
  uint8_t correct_rate=0;
  uint8_t correct_litte=0;
  uint16_t change_rate[3]={0,0,0};//����Ư�Ƶ����Ƴ̶�
  uint16_t change_value[3]={0,0,0};//��������Ư�ƵĲ�ֵ
  uint16_t change_dis[3]={0,0,0};//�洢һ�����Ư��ֵ
  int16_t a_array[DIV_NUM];
  int16_t b_array[DIV_NUM];

  for (i=0; i<DIV_NUM; i++) {
    a_array[i] = XYZ[i].changes[ca];
    b_array[i] = XYZ[i].changes[cb];
  }

  correct_rate = CAR_CHANGE_RATE;//30%
  correct_litte = CAR_CHANGE_LITTE_1;//5

  for(i=0;i<3; i++)
  {
    change_value[i] = abs(a_array[i] + b_array[i]);
    change_dis[i] = (abs(a_array[i])>abs(b_array[i])) ? abs(a_array[i]) : abs(b_array[i]);
    if(change_dis[i]!=0) // && change_value[i] <= 320)
    {
      change_rate[i] = ((uint32_t)change_value[i] * 100) / change_dis[i];
    }
    else
    {
      change_rate[i]  = change_value[i];
    }
  }

  for(i=0;i<3;i++)
  {
    if(change_dis[i]>=CAR_CHANGE_BIG_1 && change_rate[i]<=CAR_CHANGE_RATE_2)
    {
      correct_rate = CAR_CHANGE_RATE_1;
    }
   }

  for(i=0;i<3;i++)
  {
    if(change_dis[i]>=CAR_CHANGE_BIG_2 && change_rate[i]<=CAR_CHANGE_RATE)
    {
      correct_litte = CAR_CHANGE_LITTE_2;
    }
  }

  for(i=0;i<3;i++)
  {
    if(change_rate[i] <= correct_rate)
    {
      j++;
    }
    else if(abs(a_array[i]) <= CAR_CHANGE_LITTE_THRSH || abs(b_array[i]) <= CAR_CHANGE_LITTE_THRSH)
    {
      if(change_value[i] <= correct_litte)
      {
        j++;
      }
    }
  }

  //add a case to decision whether need to update baseline
  //if(ca==CAR_ENTER && cb==CAR_LEAVE)
  {
    for(i=0;i<3;i++)
    {
      if((change_dis[i]>=CAR_CHANGE_BIG_1) && (change_rate[i] <= CAR_CHANGE_RATE_2))
      {k=k+2;}
      else if ((change_dis[i]< CAR_CHANGE_BIG_1) && (change_rate[i] <= CAR_CHANGE_RATE_2))
      {k++;}
    }
    if(k>=4)
    {bs_flag=true;}
  }

#if (defined BS_DEBUG) && (BS_DEBUG == true)
  if(j>=2)
  {
    flags_value.data.thresMatch = 1;
  }
#endif
  return j;
}

/*********************************************************************
* @fn      Is_Below_Thresh
*
* @brief   check wheather it is below the threshold
*
* @param   one_thresh - a bigger threshold
*          two_thresh - a smaller threshold
*
* @return  below status
*/
uint8_t Is_Below_Thresh(uint8_t one_thresh, int8_t bs_branch)
{
  uint16_t t1=0;
  uint16_t t2[3]={0,0,0};
  uint8_t be_status=0,i=0;
  for(i=0;i<DIV_NUM;i++)
  {
    switch (bs_branch) {
      case SMOOTH_BS:
        t2[i] = abs(XYZ[i].short_smooth - XYZ[i].base_line); break;
      case ADAPTIVE_BS:
        t2[i] = abs(XYZ[i].short_smooth - XYZ[i].adaptive_base_line); break;
      case SETUP_BS:
        t2[i] = abs(XYZ[i].short_smooth - XYZ[i].setup_base_line); break;
    }
    t1 += t2[i];
  }

  if(t1 < one_thresh)
  {
    be_status=1;
  }
  return be_status;
}

/*********************************************************************
* @fn      Adaptive_Sampling
*
* @brief   Compute the sample period value according to the magnetic field changed.
*
* @param   none
*
* @return  none
*/
void Adaptive_Sampling()
{
  if(!signal.stable && signal.stable!=signal.latest_stable) //from stable to unstable
  {
    Set_AMR_Period(signal.disturb_sample_freq); // faster sample
#if DEBUG_VD
  printf("disturb_sample\n");
#endif
    //Set_AMR_Period(CLOCK_SECOND/20); // faster sample
  }
  else if(signal.stable && signal.stable!=signal.latest_stable) //from unstable to stable,must set the continue faster sample number
  {
    signal.continue_sample_num=15;  //�ȶ���������ٲ����Ĵ���
  }
  else if(signal.stable && signal.stable==signal.latest_stable)
  {
    if(signal.continue_sample_num==1)
    {
      //Set_AMR_Period(PARKINGAPP_SEND_PERIODIC_MSG_TIMEOUT); //slow sample
      Set_AMR_Period(signal.stable_sample_freq); //slow sample
      //Set_AMR_Period(CLOCK_SECOND/20); //slow sample
 #if DEBUG_VD
  printf("stable_sample\n");
#endif
    }
    if(signal.continue_sample_num>0)
    {
      signal.continue_sample_num--;
    }

  }

}

/*********************************************************************
* @fn      Average_Over_Thresh
*
* @brief   check wheather the average value is over the thresh
*
* @param   a_thresh - the thresh
*
* @return  over status
*/
uint8_t Average_Over_Thresh(uint8_t a_thresh)
{
  uint8_t t_status=0, i=0;
  uint16_t t_value[3] = {0, 0, 0};
  for(i=0; i<3; i++)
  {
    t_value[i] = abs(XYZ[i].short_smooth -XYZ[i].base_line);
  }
  if (Over_Main_Thresh(t_value[0], t_value[1], t_value[2], a_thresh))
  {
    t_status = 1;
  }
  return t_status;
}

/*********************************************************************
* @fn      base_is_all_zero
*
* @brief   check whether the baseline all is zero
*
*
* @param   one_thresh - a bigger threshold
*          two_thresh - a smaller threshold
*
* @return  bool flag
*/
/*
static bool base_is_all_zero(void)
{
  uint8_t i;
  for (i = 0; i < DIV_NUM; i++)
    if (XYZ[i].base_line != 0)
      return false;
  return true;
}
*/

/*********************************************************************
* @fn      Is_Over_Thresh
*
* @brief   check wheather it is over the thresh
*
* @param   CURR_SINGLE_THRSH - a bigger threshold
*          CURR_DOUBLE_THRSH - a smaller threshold
*          CURR_THIRD_THRSH - a smallest threshold
*
* @return  over status
*/
uint8_t Is_Over_Thresh(uint8_t CURR_SINGLE_THRSH, uint8_t CURR_DOUBLE_THRSH, uint8_t CURR_THIRD_THRSH)
{
  uint8_t a_status=0, i=0;
  uint16_t t_value[3] = {0, 0, 0};

  for(i=0; i<3; i++)
  {
    //t_value[i] = abs(XYZ[i].smooth_value - XYZ[i].base_line);
    t_value[i] = abs(XYZ[i].short_smooth - XYZ[i].base_line);
  }

  // smooth_value - base_line ����һ���ᳬ����ֵCURR_SINGLE_THRSH
  // �����������ᳬ����ֵCURR_DOUBLE_THRSH
  // ��ô����Ϊ���г�״̬
  if (Over_Main_Thresh(t_value[0], t_value[1], t_value[2], CURR_SINGLE_THRSH))
  {
    a_status = 1;
  }
  else if (Space_Status(t_value[0], t_value[1], CURR_DOUBLE_THRSH, CURR_THIRD_THRSH))
  {
    a_status = 1;
  }
  else if (Space_Status(t_value[1], t_value[2], CURR_DOUBLE_THRSH, CURR_THIRD_THRSH))
  {
    a_status = 1;
  }
  else if (Space_Status(t_value[0], t_value[2], CURR_DOUBLE_THRSH, CURR_THIRD_THRSH))
  {
    a_status = 1;
  }
  else
  {
    a_status = 0;
  }

  return a_status;
}

/*********************************************************************
* @fn      Over_Main_Thresh
*
* @brief   Compute the stable status of the node
*
* @param   none
*
* @return  none
*/
bool Over_Main_Thresh(uint16_t t0, uint16_t t1, uint16_t t2, uint8_t main_thresh)
{
  bool m_status=false;
  if(t0 >= main_thresh || t1 >= main_thresh || t2 >= main_thresh)
  {
    m_status=true;
  }

  return m_status;
}

/*********************************************************************
* @fn      XYZ_IS_Fluctuation
*
* @brief   check the signal of x, y, z has a fluctuation
*          if raw_value - smooth_value > IGNORE_THRESH
*          or raw_value - hist_raw_value > IGNORE_THRESH
*          the signal is unstable
* @param   none
*
* @return  none
*/
void XYZ_IS_Stable()
{
  uint8_t i=0;
  uint16_t t1[DIV_NUM]={0};
  //uint16_t t2[DIV_NUM]={0};
  uint8_t COUNT_NUM=0;

  for(i=0; i<DIV_NUM; i++)
  {
    //B23 2012-09-03 �����źŲ����ж�����
    //Case1: ��ǰ����Ϊs(i), ��ô����N������XYZ[i].raw_value - XYZ[i].base_line<T1,����Ϊ�ź�ƽ�ȣ�N>=3��Z[i].stable=1;
    //t2[i]=abs(XYZ[i].raw_value - XYZ[i].base_line);
    /*
    t2[i]=abs(XYZ[i].short_smooth - XYZ[i].base_line);
    if(t2[i] < XYZ[i].stable_thresh)
    {
      XYZ[i].unocc_count++;
    }
    else
    {
      XYZ[i].unocc_count=0;
    }
    if(XYZ[i].unocc_count>=LEAVING_COUNT) //3 //SHORT_LEN
    {
      XYZ[i].axis_stable = true; //�������������������ڶ��ּ��
      XYZ[i].unocc_count=LEAVING_COUNT; //SHORT_LEN
      continue;
    }
    */
    //Case2: ��ǰ����Ϊs(i), ��ô����N������XYZ[i].raw_value - XYZ[i].base_line > T1,����Ҫ��������ȥ�ж�
    t1[i]=abs(XYZ[i].raw_value - XYZ[i].smooth_value);
    //t1[i]=abs(XYZ[i].short_smooth - XYZ[i].smooth_value);
    if(t1[i] >= XYZ[i].stable_thresh)
    {
      XYZ[i].qi_count++; //qi_countͳ���������ڲ�����ֵ�ĸ���
      XYZ[i].wen_count=0;
    }
    else// if (t1[i] <= 5)
    {
      XYZ[i].qi_count=0;
      XYZ[i].wen_count++; //wen_countͳ���������ڲ�����ֵ�ĸ���
    }
    //else
    //{
    //  XYZ[i].qi_count=0;
    //  XYZ[i].wen_count=0;
    //}

    if(XYZ[i].axis_stable)
    {
      if(XYZ[i].qi_count>=1) //3
      {
        XYZ[i].axis_stable = false;
        XYZ[i].qi_count=0;
      }
    }
    else
    {
      //if(signal.status==0)
        COUNT_NUM=30; //if the parking is unoccupied, delay
      //else
      //  COUNT_NUM=20; //if the parking is occupied, faster
      //����ӳ�3�룬wen_count=3*1000/Faster_period
      if(XYZ[i].wen_count>=COUNT_NUM) //if the axis is stable countinus wen_count, then it is stable
      {
        XYZ[i].axis_stable = true;
        XYZ[i].wen_count=0;
      }
    }
  }
}

/*********************************************************************
* @fn      XYZ_IS_Stable
*
* @brief   check when the fluctuation return to stable
*          if all samples in the smooth buffer meet the condition
*          sample[k] - smooth_vale <IGNORE_THRESH, the signal is stable
* @param   none
*
* @return  none
*/
/*
void XYZ_IS_Stable()
{
  uint8_t i=0;
  uint16_t t1[DIV_NUM]={0};
  uint16_t t2[DIV_NUM]={0};

  for(i=0;i<DIV_NUM;i++)
  {
    if (!XYZ[i].axis_stable)
    {
      t1[i]=abs(XYZ[i].raw_value - XYZ[i].hist_raw_value);

      //Case1: ��ǰ����Ϊs(i), ��ô����M������s(i)-baseline<T1,����Ϊ�ź�ƽ�ȣ�M>=3,��z[i].stable=1��
      if(t1[i] < XYZ[i].stable_thresh)
      {
        XYZ[i].wen_count++;
      }
      else
      {
        XYZ[i].wen_count=0;
      }
      if(XYZ[i].wen_count>=15)
      {
        XYZ[i].axis_stable = true;
        XYZ[i].wen_count=0;
      }
      //Case2: ��ǰ����Ϊs(i), ��ô����N������s(i)-s(i-1)<T1,����Ϊ�ź�ƽ�ȣ�N>=10��Z[i].stable=1;
      if(t2[i] < XYZ[i].stable_thresh)
      {
        XYZ[i].unocc_count++;
      }
      else
      {
        XYZ[i].unocc_count=0;
      }
      if(XYZ[i].unocc_count>=3)
      {
        XYZ[i].axis_stable = true;
        XYZ[i].unocc_count=0;
      }
    }
  }
}
*/
/*********************************************************************
* @fn      Signal_IS_Stable
*
* @brief   check whether the signal is stable
*
*
* @param   none
*
* @return  none
*/
void Signal_IS_Stable()
{
  uint8_t i=0, unstable_count=0;;

  if(signal.stable && bs_lock_flag)
  {
    bs_lock_count++;
    if(bs_lock_count>=BS_LOCK_NUM) //50
    {
      bs_lock_flag=false;
      bs_lock_count=0;
    }
  }

  for(i=0;i<DIV_NUM;i++)
  {
    if(!XYZ[i].axis_stable)
    {
      unstable_count++;
    }
  }

  //at least two axis value is unstable
  if(unstable_count>=1)
  {
    //the signal is from stable go into fluctuate
      if(signal.stable)
      {
        signal.stable=false;
      }
  }
  else if(unstable_count==0)
  {
    //the signal is from fluctuate go into stable
    if(!signal.stable)
    {
      signal.stable=true;
    }
  }
}
void Statistic_Wave_Shape()
{
  if(!signal.stable)
  {
    Set_Hill_Valley(); // get hill and valley
  }

  //Ϊ����is_thunder[]����
  //ͨ������������is_thunder
  if(signal.stable && signal.thunder_count_flag)
  {
    signal.thunder_count++;
    Reset_Is_Thunder();
  }
  if(!signal.stable && signal.stable!=signal.latest_stable) //from stable to unstable
  {
    //signal.before_fluctuate_flag=true;
    Set_Before_Fluctuate();
    signal.adaptive_bs_wait_num=0;
  }

  else if(signal.stable && signal.stable!=signal.latest_stable) //from unstable to stable
  {
    //signal.after_fluctuate_flag=true;
    Set_After_Fluctuate();
  }
}
/*********************************************************************
* @fn      Set_Before_Fluctuate
*
* @brief   functions used before a start of a fluctuation
*
*
* @param   none
*
* @return  none
*/
void Set_Before_Fluctuate()
{
  //uint8_t i=0;
  //signal.before_fluctuate_flag = false;

  //����ǰ������ʼ��
  //Set_Variant_Init();
  /*
  if(!init_base_line_flag)
  {
    for(i=0;i<DIV_NUM;i++)
    {
      XYZ[i].setup_base_line = XYZ[i].base_line;
    }
    init_base_line_flag=true;
  }
  */
#if UP_HILLS==1
  Init_Upload_Struct(); //init wave struct
#endif
  //����ǰƽ��ֵ����ȡ
  Set_Before_Smooth();

  //����ǰ��ֵ������
  Set_Before_Hill();
#if DEBUG_OUT==1
  //WriteWaveInfo(1);
#endif
}
/*********************************************************************
* @fn      Set_Before_Smooth
*
* @brief   set history smooth value
*
*
* @param   none
*
* @return  none
*/
void Set_Before_Smooth()
{
  uint8_t i=0;
  signal.st_before_fluct=signal.status;//��¼����ǰ��״̬
  for(i=0;i<DIV_NUM;i++)
  {
    XYZ[i].hist_smooth_value = XYZ[i].delay_smooth;//��ʱ��smooth_valueӦ���Ѿ������˼�����������
  }
}

/*********************************************************************
* @fn      Set_Before_Hill
*
* @brief   set hill value
*
*
* @param   none
*
* @return  none
*/
void Set_Before_Hill()
{
  uint8_t i=0,j=0;
  for(j=0; j<DIV_NUM; j++)
  {
    XYZ[j].hill_point=0;
    for(i=0;i<HILL_LENGTH;i++)
    {
      XYZ[j].hill_valley[i]= 0;
    }
    XYZ[j].hill_valley[0]=XYZ[j].smooth_value;
#if UP_HILLS==1
    wave.upload_hill[up_point][j]=XYZ[j].smooth_value;
#endif
    XYZ[j].hill_point++;
    XYZ[j].big_num=0;
    XYZ[j].temp_hill=0;
    XYZ[j].little_num=0;
    XYZ[j].hill_count=0;
    XYZ[j].big_amplitute=0;
  }
  if(signal.status==0)
  {
    signal.is_thunder[0] = false;
    signal.is_thunder[1] = false;
  }
#if UP_HILLS==1
  up_point++;
#endif
}

/*********************************************************************
* @fn      Set_After_Fluctuate
*
* @brief   Call functions after a fluctuate
*
*
* @param   none
*
* @return  none
*/
void Set_After_Fluctuate()
{
  //after_fluctuate_flag=false;

  //������ƽ��ֵ��ȡ
  //Set_After_Smooth();

  //��ֹ���ж�Ϊͣ��ǰ����base_line
  bs_lock_flag=true;
  bs_lock_count=0;

  //������ֵ������
  Set_After_Hill();

  //ͳ�Ʋ��岨��num��hill vellay����,and max hill_vellay 
  Calculate_Amplitude();

  //����is_thunder[]
  Check_Hill_Amplitude();
  //set value for upload_wave struct
#if UP_HILLS==1
  Set_Upload_Data(3, 0);
#endif

  //�������������������
  //Unfluctuate_Parking();

  //����ı���
  Compute_Change();
#if DEBUG_OUT==1
  //WriteWaveInfo(2);
#endif
}
/*********************************************************************
* @fn      Unfluctuate_Parking
*
* @brief   there doesn't has a fluctuate, but the value is bigger change
*
*
* @param   none
*
* @return  none
*/
void Unfluctuate_Parking()
{
  if(!signal.is_thunder[0] && signal.status != 1)
  {
    if(Smooth_Base_Status(SINGLE_THRSH_B)==1)
    {
      signal.unfluct_parking=true;
    }
  }
}

/*********************************************************************
* @fn      Set_After_Hill
*
* @brief   set hill value
*
*
* @param   none
*
* @return  none
*/
void Set_After_Hill()
{
  uint8_t i=0;

  for(i=0;i<DIV_NUM;i++)
  {
    //if((XYZ[i].big_num > 3 || XYZ[i].little_num >3) && XYZ[i].hill_point < HILL_LENGTH)
    //{
    //  XYZ[i].hill_valley[XYZ[i].hill_point]=XYZ[i].temp_hill;
    //  XYZ[i].hill_point ++;
    //}

    if(abs(XYZ[i].smooth_value - XYZ[i].hill_valley[XYZ[i].hill_point-1]) > 10 && XYZ[i].hill_point < HILL_LENGTH) //20
    {
      XYZ[i].hill_valley[XYZ[i].hill_point] = XYZ[i].short_smooth; //XYZ[i].smooth_value;
      XYZ[i].hill_point++;
    }
#if UP_HILLS==1
    if(up_point < UPLOAD_LEN) //10
    {
      wave.upload_hill[up_point][i]=XYZ[i].short_smooth; //XYZ[i].smooth_value;
    }
#endif
  }
#ifdef DEBUG_OUT_1
  fputs(", hill: ", outfile);
  for(i=0;i<XYZ[AX_INDEX].hill_point;i++)
  {
    WriteOutFile(XYZ[AX_INDEX].hill_valley[i], " ", outfile);
  }
#endif
}

/*********************************************************************
* @fn      Set_Hill_Valley
*
* @brief   setting hill and valley values
*
*
* @param   none
*
* @return  none
*/
void Set_Hill_Valley()
{
  uint8_t hill_point=0, i=0;
  uint16_t hill_dis=0;
  for(i=0;i<DIV_NUM;i++)
  {
    hill_point = XYZ[i].hill_point;
    if(hill_point >= HILL_LENGTH) //20
    {
      break;
    }
    //if (XYZ[i].short_smooth > XYZ[i].hist_short_smooth)
    if (XYZ[i].raw_value > XYZ[i].hist_raw_value)
    {
      XYZ[i].big_num++;
      if(XYZ[i].big_num >= NUM_THRESH) //б������NUM_THRESH��>0  //2
      {
        if(XYZ[i].little_num > 0)
        {
          if(XYZ[i].little_num >= NUM_THRESH) //֮ǰ�Ѿ����ֹ�б������NUM_THRESH��<0
          {
            hill_dis=abs(XYZ[i].temp_hill-XYZ[i].hill_valley[hill_point-1]);
            if(hill_dis > SLOPE_THRESH) //��ֵ����>slope_thresh //15
            {
              XYZ[i].hill_valley[hill_point] = XYZ[i].temp_hill; //��ô�Ǽ�Сֵ��
              XYZ[i].hill_point++;
#if UP_HILLS==1
              Set_Upload_Data(1,i);
#endif
            }
          }
          XYZ[i].little_num=0;
        }
        //XYZ[i].temp_hill = XYZ[i].short_smooth;
        XYZ[i].temp_hill = XYZ[i].raw_value;
#if UP_HILLS==1
        Set_Upload_Data(2,i);
#endif
      }
    }
    //else if (XYZ[i].short_smooth < XYZ[i].hist_short_smooth)
    else if (XYZ[i].raw_value < XYZ[i].hist_raw_value)
    {
      XYZ[i].little_num++;

      if(XYZ[i].little_num >= NUM_THRESH) //б������NUM_THRESH��<0
      {
        if(XYZ[i].big_num > 0)
        {
          if(XYZ[i].big_num >= NUM_THRESH) //֮ǰ�Ѿ����ֹ�б������NUM_THRESH��>0
          {
            hill_dis=abs(XYZ[i].temp_hill-XYZ[i].hill_valley[hill_point-1]);
            if(hill_dis > SLOPE_THRESH)
            {
            XYZ[i].hill_valley[hill_point] = XYZ[i].temp_hill; //��ô�Ǽ���ֵ��
            XYZ[i].hill_point++;
#if UP_HILLS==1
          Set_Upload_Data(1,i);
#endif
            }
          }
          XYZ[i].big_num=0;
        }
        //XYZ[i].temp_hill = XYZ[i].short_smooth;
        XYZ[i].temp_hill = XYZ[i].raw_value;
#if UP_HILLS==1
        Set_Upload_Data(2,i);
#endif
      }
    }
  }

}

//ͳ�Ʊ仯����
//ͳ�Ʋ�������
/*********************************************************************
* @fn      Calculate_Amplitude
*
* @brief   calculate the number of fluctuation and amplitude over their threshold
*
*
* @param   none
*
* @return  none
*/
void Calculate_Amplitude()
{
  uint8_t i=0, j=0, k=0, bk=0, plen=0;
  int8_t temp_hill[HILL_LENGTH]={0};
  int16_t slope=0;
  int16_t max_min[2]={0}; // max_min[0] store the min value, max_min[1] store the max value
  for(i=0;i<DIV_NUM;i++)
  {
    plen = XYZ[i].hill_point;
    if(plen>1)
    {
      max_min[0]=XYZ[i].hill_valley[0];
      max_min[1]=XYZ[i].hill_valley[0];
    }
    for(j=1;j<plen;j++)
    {
      if(XYZ[i].hill_valley[j] < max_min[0]) // search the min value
      {
        max_min[0] = XYZ[i].hill_valley[j];
      }

      if(XYZ[i].hill_valley[j] > max_min[1]) //search the max value
      {
        max_min[1] = XYZ[i].hill_valley[j];
      }

      slope = XYZ[i].hill_valley[j] - XYZ[i].hill_valley[j-1];
      if(abs(slope) >= SLOPE_THRESH) //15
      {
        if(slope>0)
        {
          temp_hill[k] = 1;
          k++;
        }
        else if(slope < 0 )
        {
          temp_hill[k]=-1;
          k++;
        }
        if(abs(slope) >= BIG_SLOPE_THRESH) // 30
          bk++;
      }
    }
    XYZ[i].max_min_amplitute=abs(max_min[1]-max_min[0]);
    max_min[0]=0;
    max_min[1]=0;
    XYZ[i].big_amplitute = bk;
    if(abs(temp_hill[0])==1)
      XYZ[i].hill_count++;

    for(j=1;j<k;j++)
    {
      if((temp_hill[j] + temp_hill[j-1])==0)
        XYZ[i].hill_count++;
    }
#ifdef DEBUG_OUT_1
    if(i==AX_INDEX)
      fputs(", fluct: ", outfile);
#endif
    for(j=0;j<HILL_LENGTH;j++)
    {
#ifdef DEBUG_OUT_1
      if(i==AX_INDEX && j<k)
        WriteOutFile(temp_hill[j], " ", outfile);
#endif
      temp_hill[j]=0;
    }

    k=0;
    bk=0;
  }
#ifdef DEBUG_OUT_1
  WriteOutFile(XYZ[AX_INDEX].hill_count, ", hill_count ", outfile);
  WriteOutFile(XYZ[AX_INDEX].big_amplitute, ", big_amplitute ", outfile);
#endif
}

#if UP_HILLS==1
/*********************************************************************
* @fn      Set_Upload_Data
*
* @brief   set data for upload_wave struct, include base_line, smooth_value,
*          hill and valley, and decide weather need to upload the message
*
* @param   none
*
* @return  none
*/
void Set_Upload_Data(uint8_t branch, uint8_t index)
{
  uint8_t p=0, k=0;

  switch (branch)
  {
    case 1:
      if(up_point < UPLOAD_LEN)
      {
        for(k=0; k<DIV_NUM; k++)
        {
          if (wave.upload_hill[up_point-1][k]==temp_hill[index][k])
          {
            p++;
          }
        }
        if(p>=DIV_NUM) // ��ǰ�ķ�ֵ��������ʷ����һ�£�����
        {
          break;
        }
        for(k=0; k<DIV_NUM; k++)
        {
          wave.upload_hill[up_point][k]=temp_hill[index][k];
        }
        up_point++;
      }
    case 2:
      temp_hill[index][0]=XYZ[0].raw_value;
      temp_hill[index][1]=XYZ[1].raw_value;
      temp_hill[index][2]=XYZ[2].raw_value;
      break;
    case 3:
      //��ʾ������������ֵ����
      //����Ҫ�ϴ�
      if(up_point >= LEAST_HILL_COUNT) //3
      {
        up_hill_changed = 1;
       // up_delay_count = 9 + (random_rand() % 5);
        up_delay_count = 9 + 5;
        //call_upload_wave(&wave);
        //WriteUploadInfo();
      }
      break;
  }
}

//�ӳٷ��Ͳ�������
//���������복λ״̬�ı�ܴ������ͬʱ������
//���û���ӳ���������������ͻ�����״̬�ı����ݶ�ʧ
void delay_call_up()
{
  uint8_t k=0;
  if (up_hill_changed==1)
  {
    if(up_delay_count>0)
    {
      up_delay_count--;
    }
    else if(up_delay_count==0)
    {
      up_hill_changed = 0;
      if((max_min_value>=100) || (signal.st_before_fluct!=signal.status))
      {
//      fputs("\r\n uploadwave\n", outfile);
      for(k=0;k<DIV_NUM;k++)
      {
        wave.upload_bs[k]=XYZ[k].adaptive_base_line; // the base line value
        wave.upload_smooth[k]=XYZ[k].short_smooth; // the current smooth value of magnetic signal
      }
      wave.judge_branch=enter_leave_branch;
      wave.status=signal.status;
//      WriteUploadInfo();
 //     Set_UP_HILLS(&wave);
      }
    }
  }
}
/*********************************************************************
* @fn     Init_Upload_Struct
*
* @brief   set data for upload_wave struct, include base_line, smooth_value,
*          hill and valley, and decide weather need to upload the message
*
* @param   none
*
* @return  none
*/
void Init_Upload_Struct()
{
  uint8_t i=0,j=0;
  for(i=0;i<UPLOAD_LEN;i++) //10
  {
    for(j=0;j<DIV_NUM;j++) //3
    {
      wave.upload_hill[i][j]=0;
    }
  }
  for(i=0;i<DIV_NUM;i++)
  {
    wave.upload_bs[i]=0;
    wave.upload_smooth[i]=0;
  }
  //info->status=0;
  up_point=0;
}
#endif
//����ͣ�����
/*********************************************************************
* @fn      IS_Parking
*
* @brief   vehicle parking detection
*
*
* @param   none
*
* @return  none
*/
void IS_Parking()
{
  if(!signal.stable)
  {
    current_thresh = signal.occ_big_threshold;
    enter_branch_num=1;
    need_parking_count=15;
  }
  else
  {
    if(signal.is_thunder[0])
    {
      current_thresh=signal.occ_little_threshold;
      enter_branch_num=2;
      need_parking_count=CAR_ARRIVAL_COUNT; //5
    }
    else
    {
      current_thresh = signal.occ_mid_threshold;
      enter_branch_num=3;
      need_parking_count=CAR_ARRIVAL_COUNT;
    }
  }

  signal.b_status = Smooth_Base_Status(current_thresh); // 

  if(signal.b_status ==1)
  {
    signal.below_count=0;
  }
  if(signal.status != 1 && signal.b_status == 1)
  {
    signal.over_count++;
    if(signal.over_count >= need_parking_count)
    {
      signal.over_count = 0;
      signal.compute_long_bs=true; //need to compute the value of (long_stable_value - base_line)
      Car_Arrival_Functions(enter_branch_num);
      //set_route_change(bs_change);
      //fputs(", Enter 1!\n", outfile);
    }
  }

}
//the function is canceled
//����Ư���ж��Ƿ��г�������
uint8_t Entering_Drift()
{
  int i=0,k=0;
  for (i=0; i<DIV_NUM; i++)
  {
    if(XYZ[i].changes[CAR_ENTER]>=20)
    {k=k+2;}
    else if (XYZ[i].changes[CAR_ENTER]>=15)
    {k++;}
  }
  if(k>=4)
  {
    return 1;
  }
  return 0;
}

/*********************************************************************
* @fn      IS_Leaving
*
* @brief   vehicle leaving detection
*
*
* @param   none
*
* @return  none
*/
void IS_Leaving()
{
  //int16_t a_array[3];
  uint8_t f_leaving_branch=0;
  //bool need_out=false;
  signal.b_status = Smooth_Base_Status(current_thresh);
  //signal.b_status = Smooth_Base_Status(current_single, current_double, current_third);
  //if(signal.status!=0 && signal.b_status==0 && signal.stable)
  //�����ź��ȶ�״̬Ҳ�б�
  if(signal.status!=0 && signal.b_status==0)
  {
    //check leave using smooth_base_line
    signal.fuse_status = Is_Below_Thresh(signal.unocc_threshold, SMOOTH_BS);
    if(signal.fuse_status== 1 )
    {
      f_leaving_branch=1;
    }
    else
    {
      //check leave using adaptive_base_line
      signal.fuse_status = Is_Below_Thresh(signal.unocc_threshold, ADAPTIVE_BS);
      if(signal.fuse_status==1)
      {
        f_leaving_branch=2;
      }
    }

    if(signal.fuse_status==1)
    {
      signal.below_count++;
      if(signal.below_count >= CAR_LEAVE_COUNT) //5
      {
        Car_Leave_Functions(f_leaving_branch);
      }
    }
    else
    {
      signal.below_count=0;
    }
  }
  //else if(is_thunder[1] && b_status==1 && (!UpdateLock))
  if(signal.status!=0 && signal.stable)
  {
    if(!base_enter_change_is_all_zero() && ! base_leave_change_is_all_zero())
    {
      if(Is_Change_Sim(BASE_ENTER, BASE_LEAVE))
      {
        Car_Leave_Functions(3);
        return;
      }
      else
      {
       XYZ[0].changes[BASE_LEAVE]=0;
       XYZ[1].changes[BASE_LEAVE]=0;
       XYZ[2].changes[BASE_LEAVE]=0;
      }
    }
    if(!car_leave_change_is_all_zero() && ! car_enter_change_is_all_zero())
    {
      if(Is_Change_Sim(CAR_ENTER, CAR_LEAVE))
      {
        Car_Leave_Functions(4);
        return;
      }
      else
      {
       XYZ[0].changes[CAR_LEAVE]=0;
       XYZ[1].changes[CAR_LEAVE]=0;
       XYZ[2].changes[CAR_LEAVE]=0;
      }
    }

    if(!base_enter_change_is_all_zero() && ! long_leave_change_is_all_zero())
    {
      if(Is_Change_Sim(BASE_ENTER, LONG_LEAVE))
      {
        Car_Leave_Functions(5);
        return;
      }
      else
      {
       XYZ[0].changes[LONG_LEAVE]=0;
       XYZ[1].changes[LONG_LEAVE]=0;
       XYZ[2].changes[LONG_LEAVE]=0;
      }
    }
    /*
    if(signal.new_change_flag && (!new_leave_change_is_all_zero()))
    {
      need_out = Base_Smooth_Check();
      //B06 2012-10-04
      //������need_out�������п���New_Change
      if(need_out)
      {
        car_change_i = Is_Car_Leaved(NEW_CAR_ENTER, NEW_CAR_LEAVE);
        if(car_change_i >= 3)
        {
          Car_Leave_Functions(3);
        }
      }
    }
    */
  }
}
/*********************************************************************
* @fn      Base_Smooth_Check
*
* @brief   check wheather need out
*
* @param   none
*
* @return  none
*/
bool Base_Smooth_Check(void)
{
  bool need_out=true;
  uint8_t i=0, j=0, k=0;
  for(i=0;i<3;i++)
  {
    if(abs(XYZ[i].base_line - XYZ[i].smooth_value)>= BASE_SMOOTH_1)
    {
      j++;
      if(abs(XYZ[i].base_line - XYZ[i].smooth_value)>= BASE_SMOOTH_2)
      {
        k++;
      }
    }
  }
  if(j>=2 && k >=1)
  {
    need_out = false;
  }
  return need_out;
}
//��ʼ�������
/*********************************************************************
* @fn      Check_Hill_Amplitude
*
* @brief   Check whether the signal is satisfy the car arrival or leaving
*
*
* @param   none
*
* @return  none
*/
void Check_Hill_Amplitude()
{
  uint8_t i=0, hill_count=0, big_amplitute=0;
  max_min_value=0;
  for(i=0;i<DIV_NUM;i++)
  {
    hill_count += XYZ[i].hill_count;
    big_amplitute += XYZ[i].big_amplitute;
    max_min_value += XYZ[i].max_min_amplitute;
  }

  if(hill_count >= OVER_THUNDER_COUNT && big_amplitute >= OVER_THUNDER_BIG && max_min_value >= MAX_MIN_THRESH) /// 6  3  180
  {
    //if(!signal.is_thunder[0] && signal.status==0)
    //���ź��ȶ�ǰ�������Ѿ������ټ�ⷽ�����ж�ͣ��
    //���ź��ȶ�������Ҫ����is_thunder[0]
    if(!signal.is_thunder[0])
    {
      signal.is_thunder[0] = true;
      signal.thunder_count=0;
      signal.thunder_count_flag = true;
    }
    //else if(signal.is_thunder[0] && !signal.is_thunder[1] && signal.status!=0)
    else if(signal.is_thunder[0] && !signal.is_thunder[1])
    {
      signal.is_thunder[1] = true;
      signal.thunder_count=0;
      signal.thunder_count_flag = true;
    }
  }
#ifdef DEBUG_OUT
  WriteOutFile(hill_count, ", s.hill_ct ", outfile);
  WriteOutFile(big_amplitute, ", s.big_at ", outfile);
  WriteOutFile(max_min_value, ", s.max_min ", outfile);
  WriteOutFile(signal.is_thunder[0], ", T0 ", outfile);
  WriteOutFile(signal.is_thunder[1], ", T1 ", outfile);
#endif

}

/*********************************************************************
* @fn      Reset_Is_Thunder
*
* @brief   reset the values of the array of is_thunder
*          when is_thunder[0]=1 and usually then signal.status=1
*          ���is_thunder[0]=1 ����һ��ʱ��� status=0, ������is_thunder
* @param   none
*
* @return  none
*/
void Reset_Is_Thunder()
{
  if(signal.thunder_count > RESET_THUNDER_COUNT) //25
  {
    signal.thunder_count_flag = false;
    if(signal.status != 1)
    {
      signal.is_thunder[0] = false;
      signal.is_thunder[1] = false;
    }
  }
}

/*********************************************************************
* @fn      changes_is_all_zero
*
* @brief   check whether the value of changes is zero
*
*
* @param   none
*
* @return  none
*/
static bool changes_is_all_zero(uint8_t cId)
{
  uint8_t i;
  for (i = 0; i < DIV_NUM; i++)
    if (XYZ[i].changes[cId] != 0)
      return false;
  return true;
}


/*********************************************************************
* @fn      magnetic_change_match
*
* @brief   check difference of the value of changes
*
*
* @param   none
*
* @return  none
*/
static bool magnetic_change_match(uint8_t ca, uint8_t cb)
{
  bool a_error = false;
  uint8_t i=0, a_count=0, b_count=0;
  uint16_t a_change=0;
  int16_t a_array[3];
  int16_t b_array[3];

  // prepare array data
  for (i = 0; i < 3; i++) {
    a_array[i] = XYZ[i].changes[ca];
    b_array[i] = XYZ[i].changes[cb];
  }

  for(i=0; i<3; i++)
  {
    if(abs(a_array[i]) < ERROR_THRESH) //50
    {
      a_count++;
    }
    if(abs(b_array[i]) >= ERROR_THRESH)
    {
      b_count++;
    }
  }
  if(a_count==3 && b_count>0)
  {
    for(i=0; i<3; i++)
    {
      if(abs(b_array[i]) >= ERROR_THRESH )
      {
        a_change = abs(b_array[i] + a_array[i]);
        if(a_array[i]!=0 && a_change<=320)
        {
          a_change = a_change * 100 / abs(a_array[i]);
        }
        if(a_change >= 50)
        {
          a_error=true;
        }
      }
    }
  }
  return a_error;
}
/*********************************************************************
* @fn      Deal_Arrival_Error
*
* @brief   reset the CAR_ENTER and CAR_LEAVE changes
*
*
* @param   none
*
* @return  none
*/
void Deal_Arrival_Error()
{
  uint8_t i=0;
  //fputs(" ERROR! ", outfile);
  for(i=0; i<3; i++)
  {
    //car_arrival_change[i] = car_leave_change[i];
    //car_leave_change[i] = 0;
    XYZ[i].changes[CAR_ENTER] = XYZ[i].changes[CAR_LEAVE];
    XYZ[i].changes[CAR_LEAVE] = 0;
  }
}


/*********************************************************************
* @fn      Set_New_Change
*
* @brief   set the values of NEW_CAR_ENTER and NEW_CAR_LEAVE changes
*
*
* @param   none
*
* @return  none
*/
void Set_New_Change()
{
  uint8_t i=0, same_degree=0;
  for(i=0; i<3; i++)
  {
    if(abs(XYZ[i].changes[CAR_LEAVE]) > NEW_CHANGE_THRESH)
    {
      break;
    }
  }
  if(i<3)
  {
    //if(Is_All_Zero(new_arrival_change))
    if(new_enter_change_is_all_zero()/*Is_All_Zero(new_arrival_change)*/)
    {
      same_degree = 0;
    }
    else
    {
      same_degree = Is_Car_Leaved(NEW_CAR_ENTER, CAR_LEAVE);
    }
    for(i=0;i<3;i++)
    {
      if(same_degree>=2)
      {
        XYZ[i].changes[NEW_CAR_LEAVE] = XYZ[i].changes[CAR_LEAVE];
      }
      else
      {
        XYZ[i].changes[NEW_CAR_ENTER] = XYZ[i].changes[CAR_LEAVE];
      }
    }
    signal.new_change_flag = true;
  }
}

/*********************************************************************
* @fn      Compute_Change
*
* @brief   compute the change values of the drift over the backgroud magnetic field
*          when vehicle arrival or leaving. And these values are key feature to detect vehicle leaving
*
* @param   none
*
* @return  none
*/
void Compute_Change()
{
  uint8_t i=0;
  bool is_not_arrival=false, is_not_leave=false;
  int16_t degree_change=0;//, base_change=0;
  //node restart, and get status=1 from flash, car_enter_change is zero
  //jump the compute change function
  //if(signal.status==1 && car_enter_change_is_all_zero())
  //{
    //return;
  //}
  //ȡ����base_change, �޸�Ϊ����long_stable_value�����ж�
  for(i=0;i<3;i++)
  {
    degree_change = XYZ[i].short_smooth - XYZ[i].hist_smooth_value;
    //degree_change = XYZ[i].raw_value- XYZ[i].hist_smooth_value;//����ǿ��ٽ����ȶ�״̬��short_smooth����������

    //if(signal.status != 1 && signal.b_status==1)
    if(signal.st_before_fluct != 1)
    {
      XYZ[i].changes[CAR_ENTER] = degree_change;
      //XYZ[i].changes[BASE_ENTER] = base_change;
      XYZ[i].changes[CAR_LEAVE] = 0;
    }

    else if(signal.status==1 && signal.st_before_fluct==1)
    {
      XYZ[i].changes[CAR_LEAVE] = degree_change;
      degree_change = XYZ[i].short_smooth - XYZ[i].long_stable_value;
      XYZ[i].changes[LONG_LEAVE] = degree_change;
    }
  }


  if(signal.status==1)
  {

    //CAR_LEAVE��CAR_ENTER��ö�ĸ���
    //��ô��ǰ��CAR_ENTER������Ч
    //CAR_ENTER_change = CAR_LEAVE_change
    is_not_arrival = magnetic_change_match(CAR_ENTER, CAR_LEAVE);
    if(is_not_arrival)
    {
      Deal_Arrival_Error();
    }

    //CAR_LEAVE��CAR_ENTER�ٵö�ĸ���
    //CAR_LEAVE_change����Ϊ0
    is_not_leave = magnetic_change_match(CAR_LEAVE, CAR_ENTER);
    if(is_not_leave)
    {
      for(i=0;i<3;i++)
      {
        XYZ[i].changes[CAR_LEAVE] = 0;
      }
    }
    /*
    if(!is_not_arrival && !is_not_leave)
    {
      if(Smooth_Base_Status(current_thresh,SINGLE_THRSH_1,  DOUBLE_THRSH_1)==1)
      {
        //same_degree = Is_Car_Leaved(car_arrival_change, car_leave_change);
        same_degree = Is_Car_Leaved(CAR_ENTER, CAR_LEAVE);

        if(same_degree < 2)
          //if(same_degree < 2 || (same_degree==2 && (!signal.is_thunder[1])))
        {
          Set_New_Change();//��ʱ��Ϊ����ͣ���ܿ��������У���Ϊ������һ�θı���New_Change > enter_change
          for(i=0;i<3;i++)
          {
            //car_leave_change[i] = 0;
            XYZ[i].changes[CAR_LEAVE] = 0;
          }
        }
      }
    }
    */
  }
}

/*********************************************************************
* @fn      Space_Status
*
* @brief   This function determine the status of the parking space
*          The space is occupied if double AMR magnetic field over the DOUBLE_THRSH
*
* @param   a_value, b_value -  magnetic values
*          a_index, b_index  - the index of base_line value
*
* @return  the status flag of the parking space
*/
bool Space_Status(uint16_t a_value, uint16_t b_value, uint8_t CURR_DOUBLE_THRSH, uint8_t CURR_THIRD_THRSH)
{
  bool st = false;
  if((a_value >= CURR_DOUBLE_THRSH) && (b_value >= CURR_THIRD_THRSH))
  {
    st = true;
  }
  else if((b_value >= CURR_DOUBLE_THRSH) && (a_value >= CURR_THIRD_THRSH))
  {
    st = true;
  }
  return st;
}

/*********************************************************************
* @fn      Set_Raw_Value
*
* @brief   Compute the sample period value according to the magnetic field changed.
*
* @param   none
*
* @return  none
*/
void Set_Raw_Value(void)
{
  XYZ[0].raw_value=One_Sample.x;
  XYZ[1].raw_value=One_Sample.y;
  XYZ[2].raw_value=One_Sample.z;
 // bool is_strong_mag=false;
}

// Smooth the raw signal
/*********************************************************************
* @fn      Raw_Data_Smooth
*
* @brief   smooth filter
*
*
* @param   none
*
* @return  none
*/
void Raw_Data_Smooth()
{
  uint8_t i=0;
  for (i=0; i<DIV_NUM; i++)
  {
    Raw_Signal_Smooth(&XYZ[i]);
  }
}

//ǿ�ż������Ҫreset������
uint8_t wait_and_reset()
{
 // bool is_strong_mag=false;
  signal.init_num_count++;
 // is_strong_mag=Is_Strong_Magnetic();
  if(check_constant_signal(1))//ǿ�Ÿ��Ż�δ�뿪�������ܽ����ʼ��״̬
  {
    signal.init_num_count=0;
    return EN_INIT_FAILL;
   }
   // if(signal.re_initial_flag)
  //  {
   //  Re_Calibrate_Success(false); //�����궨ʧ����Ϣ
   //   signal.re_initial_flag=false;
   // }

    // send warn msg to prompt that operation err: need to remove the qiangci
  

  //call reset function before initialize baseline
//  if(signal.init_num_count == (INIT_DEPLOYMENT_NUM-3)) //10
//  {
//#if WITH_MMC3316
//    MMC3316_Reset(SENSOR_PORT);
//#elif WITH_HMC5983 || WITH_QMC5883
//    reset_flag=true; //�Բ⣬������������
//#else
//#error "no support"
//#endif
//  }

  if(signal.init_num_count >=INIT_DEPLOYMENT_NUM)
  {
    signal.init_num_count=0;
    signal.wait_flag=true;

  }
  return EN_INIT;
}

#if (IS_SNIFFER_VERSION==0) || (ACTIVATE_RF_BY_AMR==1)
/*********************************************************************
* @fn      check_constant_signal
*
* @brief   the VD is invoked by detecting the constant magnetic signal five times in succession
*
*
* @param   activate_branch: case 1 ǿ�ż���ڵ���빤��״̬
*                           case 2 ǿ�ż���RF���빤��״̬
*
* @return  none
*/
//void check_constant_signal(uint8_t activate_branch)
bool check_constant_signal(uint8_t activate_branch)
{
  bool is_big_mag=false;
  /*
#if WITH_MMC3316

  big_value = abs(MMC3316[SENSOR_PORT].x -8000)+abs(MMC3316[SENSOR_PORT].y -8000)+abs(MMC3316[SENSOR_PORT].z -8000);
  if(big_value >= 2000)
  {
    signal.num_count++;
  }
  else
  {
    signal.num_count=0;
  }*/
//#elif WITH_HMC5983 || WITH_QMC5883
  is_big_mag=Is_Strong_Magnetic();
  if(is_big_mag) //ǿ�Ÿ��ţ���Ҫ����һ�¡�
  {
    signal.strong_num_count++;
    if(signal.strong_num_count >= STRONG_MAG_NUM) //3
    {
      signal.weak_num_count=0;
     // signal.is_strong_mag = true; // big mag
     is_big_mag = true;
    }
  }
  else
  {   
    signal.weak_num_count++;
    if(signal.weak_num_count >= WEAK_MAG_NUM)
    {
      signal.strong_num_count=0;
     // signal.is_strong_mag = false;
      is_big_mag = false;
    }
  }
  return is_big_mag;
}  


/*********************************************************************
* @fn      Reset_After_Strong_Mag
*
* @brief   �ж��Ƿ���ǿ���ź�
*
*
* @param   none
*
* @return  none
*/
void Reset_After_Strong_Mag()
{
  bool is_strong_mag=false;
  is_strong_mag=Is_Strong_Magnetic();
  if((signal.hist_is_strong_mag) && (!is_strong_mag))
  {
 //   reset_flag=true;
  }
}



void
Set_Activated_Flag(int flag)
{
  signal.invoke_flag = flag;
#if WRITE_ALGO_FLASH==1
  if(flag==1)
  {
    signal.initialize_flag=2; //��ǽڵ��Ѿ��������������flash�д洢��base_line���г�ʼ����
  }
#endif
}

void
Set_Activated_Callback(void (*f)(void))
{
  activated_cb = f;
}
#endif

/*********************************************************************
* @fn      Is_Strong_Magnetic
*
* @brief   �ж��Ƿ���ǿ���ź�
*
*
* @param   none
*
* @return  none
*/

bool Is_Strong_Magnetic()
{
  uint16_t big_value=0, big_x=0, big_y=0, big_z=0;
  big_x = abs(One_Sample.x);
  big_y = abs(One_Sample.y);
  big_z = abs(One_Sample.z);
  big_value=big_x+big_y+big_z;
  // if it's init or re_init the first two sample need to drop and then to check strong mag
  if((!signal.wait_flag && signal.initialize_flag == 0) || (!signal.wait_flag && signal.re_initial_flag == true))
  {
    if(signal.init_num_count > 2)
    {
      if((big_value>=STRONG_MAG_THRESH) || (big_x>=MAX_SIGNAL_MAG) || (big_y>=MAX_SIGNAL_MAG) || (big_z>=MAX_SIGNAL_MAG)) // 2500 1500
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  }
  else {
      if((big_value>=STRONG_MAG_THRESH) || (big_x>=MAX_SIGNAL_MAG) || (big_y>=MAX_SIGNAL_MAG) || (big_z>=MAX_SIGNAL_MAG)) // 2500 1500
      {
        return true;
      }
      else
      {
        return false;
      }
  }
  return false;
}

/*********************************************************************
* @fn      Init_process()
*
* @brief   Init setup_base_line, stable_thresh, and unoccupied_thresh
*
*
* @param   none
*
* @return  none
*/
void Init_process()
{
  uint8_t i=0;
  if(signal.init_num_count == INIT_BASELINE_NUM) //20
  {
    for(i=0;i<DIV_NUM;i++)
    {
      XYZ[i].setup_base_line = XYZ[i].smooth_value;
      XYZ[i].base_line = XYZ[i].smooth_value;
      XYZ[i].adaptive_base_line = XYZ[i].smooth_value;
      Fill_base_buf(&XYZ[i].bs_buf,XYZ[i].smooth_value);
    }
  }
  if(signal.init_num_count > INIT_BASELINE_NUM && signal.init_num_count< INIT_THRESHOLD_NUM) // 30 -- 150
  {
    dist_count++;

    for(i=0;i<DIV_NUM;i++)
    {
      dist1[i] += abs(XYZ[i].raw_value - XYZ[i].smooth_value);
      dist2[i] += abs(XYZ[i].short_smooth - XYZ[i].setup_base_line);
    }
  }

  signal.init_num_count++;

  if(signal.init_num_count >= INIT_THRESHOLD_NUM) //30
  {
    for(i=0;i<DIV_NUM;i++)
    {
      //���ڵ���·��Ӧ����dist[i]*3/2�ȽϺ���
      dist1[i] = dist1[i]*2/dist_count;
      //dist2[i] = dist2[i]*2/dist_count;
      //�Ƿ���Ҫ���ݷ�����������������ֵ������ᾫȷһЩ���������Ӽ��㸴�Ӷȣ�
      //�����Ժ�����ݷ�������Ӱ�
      if(dist1[i]>XYZ[i].stable_thresh)
      {
        XYZ[i].stable_thresh = dist1[i];
      }
//      if(dist2[i]>XYZ[i].unoccupied_thresh)
//      {
//        XYZ[i].unoccupied_thresh = dist2[i];
//      }
    }

 //   Save_Algorithm_Parameters();
    signal.init_num_count=0;
    signal.initialize_flag=1;
    if(signal.re_initial_flag)
    {
      // at here need send msg to server to prompt re_init status
//      Re_Calibrate_Success(true); //�����궨�ɹ���Ϣ ...
      signal.re_initial_flag=false;
    }
  }
}

/*********************************************************************
* @fn      Base_Line_Smooth
*
* @brief   set the value of base_line
*
* @param   none
*
*
* @return  none
*/
void Base_Line_Smooth()
{
  //int16_t refer_value = 0;
  uint8_t i=0, a_value=0;

  for(i=0;i<DIV_NUM;i++)
  {
    a_value= SMOOTH_SUM - SMOOTH_PARAM; //100 10
    if(abs(XYZ[i].delay_smooth) < NORMAL_MAX) // filter the thundering magnetic value ,1200
    {

        //XYZ[i].base_line = Compute_Number(XYZ[i].base_line, XYZ[i].smooth_value, a_value, SMOOTH_PARAM);
        XYZ[i].base_line = Compute_Number(XYZ[i].base_line, XYZ[i].delay_smooth, a_value, SMOOTH_PARAM);
        //refer_value = base_line[amr_index] * ;
//        refer_value = refer_value + (XYZ[i].smooth_value * SMOOTH_PARAM / SMOOTH_SUM);
//        XYZ[i].base_line = refer_value;

    }
  }
#if defined ( NV_INIT )
  if(init_base)
  {
     Set_Flash_Value();
  }
#endif

}

/*********************************************************************
* @fn      Raw_Signal_Smooth
*
* @brief   This function smooth the raw signal of one coordinate
*
* @param   *axis -  the pointer of one coordinate
*
*
* @return  none
*/
void Raw_Signal_Smooth(cardet_axis_t* axis)
{
  uint8_t ins;
  //short_sum��ʲô�ã�
  smooth_buf_t* smooth = & axis->smooth;
  ins = smooth->tail + 1;
  if(ins >= SMOOTH_LENGTH)
    ins = 0;
  if(ins == smooth->head)
  {
    smooth->sum -= smooth->samples[smooth->head]; // minus oldest sample from sum
    smooth->head += 1; // drop oldest sample
    if(smooth->head >= SMOOTH_LENGTH)
    {
      smooth->head = 0;
    }
  }
  smooth->samples[smooth->tail] = axis->raw_value;
  smooth->sum += axis->raw_value;
  smooth->tail = ins;

  if (smooth->tail > smooth->head)
    smooth->len = smooth->tail - smooth->head;
  else
    smooth->len = smooth->tail + SMOOTH_LENGTH - smooth->head;

  smooth->short_sum +=axis->raw_value;
  if(smooth->len <= SHORT_SMOOTH)
  {
    axis->short_smooth = smooth->short_sum / smooth->len;
  }
  else
  {
    //��Ҫ����һ����
    if(smooth->tail > SHORT_SMOOTH)
    {
      smooth->short_sum -= smooth->samples[smooth->tail - 1 - SHORT_SMOOTH];
    }
    else //��ѭ�����
    {
      uint8_t left_len = SHORT_SMOOTH - smooth->tail;
      smooth->short_sum -= smooth->samples[SMOOTH_LENGTH - 1 - left_len];
    }
    axis->short_smooth = smooth->short_sum / SHORT_SMOOTH;
  }

  if(smooth->len>0)
  {
    axis->smooth_value = smooth->sum / smooth->len;
    //��ؼ��һ�¼����Ƿ�׼ȷ
    axis->delay_smooth = (SMOOTH_LENGTH * axis->smooth_value - SHORT_SMOOTH * axis->short_smooth) / (SMOOTH_LENGTH-SHORT_SMOOTH);
  }
}


/*********************************************************************
* @fn      Compute_Number
*
* @brief   Compute number.
*
* @param   none
*
* @return  result
*/
//c_num �ǲ������������
//a_num �ǳ���
//b_num �ǳ���
int16_t Compute_Number(int16_t x1, int16_t x2, uint8_t a_num, uint8_t b_num) //base_line delay_smooth, 90 ,10
{
  int16_t t1=0, t2=0, t3=0, t4=0;;
  t1 = x1/100*a_num + x2/100*b_num;
  t2 = x1%100*a_num + x2%100*b_num;
  t3 = t2%100;
  t4 = t1 + t2/100;
  if(t3>=50)
  {
    t4++;
  }
  else if(t3<=-50)
  {
    t4--;
  }
  return t4;
}


/*********************************************************************
* @fn      _debug
*
* @brief   debug message send function
*
* @param   field - indicate which variants need send
*
* @return  none
*/
#if (defined BS_DEBUG) && (BS_DEBUG == true)
static void _debug(uint8_t field)
{
  int16_t data[3]={0,0,0};
  uint8_t i;
  for (i = 0; i < 3; i++) {
    switch (field) {
    case BASE_LINE:
      data[i] = XYZ[i].base_line; break;
    case SMOOTH_VALUE:
      data[i] = XYZ[i].smooth_value; break;
    case CAR_ENTER:
      data[i] = XYZ[i].changes[CAR_ENTER]; break;
    case CAR_LEAVE:
      data[i] = XYZ[i].changes[CAR_LEAVE]; break;
    case NEW_CAR_ENTER:
      data[i] = XYZ[i].changes[NEW_CAR_ENTER]; break;
    case NEW_CAR_LEAVE:
      data[i] = XYZ[i].changes[NEW_CAR_LEAVE]; break;
    }
  }

  if(field==LEAVE_BRANCH)
  {
    data[2] = enter_leave_branch;
  }
  else if(field==ENTER_BRANCH)
  {
    data[2] = enter_leave_branch;
  }
  else if(field==CAR_ENTER || field==CAR_LEAVE)
  {
    flags_value.data.car_change=1;
  }
  else if(field==NEW_CAR_ENTER || field==NEW_CAR_LEAVE)
  {
    flags_value.data.new_change=1;
  }
  SetDebugMsg(data, field);
}
#endif

#if DEBUG_OUT==1
/*********************************************************************
 * @fn      WriteOutFile
 *
 * @brief   write <value, string> to outfile
 *
 * @param   x_value - value
 *          strname - the string
 *          outfile - out file
 *
 * @return  none
 */
void WriteOutFile(int32 x_value, char * strname, FILE *ofile)
{
  char str[10];
  // write the baseline into outfile
  fputs(strname, ofile);
  if(x_value!=0)
  {
    //itoa(x_value, str, 10);
    sprintf(str, "%d", x_value);
    fputs(str, ofile);
  }
  else
  {
    putc('0', ofile);
  }
}

void WriteFile()
{
  uint8_t i=0;
  // write into outfile
  WriteOutFile(signal.stable, " signal.st ", outfile);
  WriteOutFile(signal.long_stable_num, " stable_num ", outfile);
  WriteOutFile(signal.status, " occupied= ", outfile);
  WriteOutFile(signal.is_thunder[0], " t0= ", outfile);

  for(i=0;i<DIV_NUM;i++)
  {
    WriteOutFile(XYZ[i].raw_value, " [raw ", outfile);
    WriteOutFile(XYZ[i].smooth_value, ", long_s ", outfile);
    WriteOutFile(XYZ[i].short_smooth, ", short_s ", outfile);
    WriteOutFile(XYZ[i].delay_smooth, ", delay_s ", outfile);
    //WriteOutFile(XYZ[i].setup_base_line, ", setup_bs ", outfile);
    WriteOutFile(XYZ[i].base_line, ", bs ", outfile);
    //WriteOutFile(XYZ[i].stable_thresh, ", stable_t ", outfile);
    //WriteOutFile(XYZ[i].unoccupied_thresh, ", unocc_t ", outfile);
    WriteOutFile(XYZ[i].axis_stable, ", stable ", outfile);
    fputs(" ]",outfile);
  }
}

void WriteWaveInfo(uint8_t wave)
{
  uint8_t i=0;
  int16_t short_delay_change=0;
  int16_t long_delay_change=0;
  fputs(sdate,outwaveinfo);
  WriteOutFile(signal.status, " occupied= ", outwaveinfo);
  WriteOutFile(signal.below_count, " bc ", outwaveinfo);
  fputs("[",outwaveinfo);
  for(i=0;i<DIV_NUM;i++)
  {
    WriteOutFile(XYZ[i].changes[BASE_ENTER], " ", outwaveinfo);
  }
  fputs("][",outwaveinfo);
  for(i=0;i<DIV_NUM;i++)
  {
    WriteOutFile(XYZ[i].changes[BASE_LEAVE], " ", outwaveinfo);
  }
  fputs("]",outwaveinfo);
  for(i=0;i<DIV_NUM;i++)
  {
    WriteOutFile(XYZ[i].raw_value, " [r ", outwaveinfo);
    WriteOutFile(XYZ[i].smooth_value, ", l ", outwaveinfo);
    //WriteOutFile(XYZ[i].short_smooth, ", short ", outwaveinfo);
    //WriteOutFile(XYZ[i].delay_smooth, ", delay ", outwaveinfo);
    WriteOutFile(XYZ[i].base_line, ", bs ", outwaveinfo);
    WriteOutFile(XYZ[i].long_stable_value, ", lsv ", outwaveinfo);
    fputs("]",outwaveinfo);
  }
  /*
  if(wave==2)
  {
    fputs(" short-delay[",outwaveinfo);
    for(i=0;i<3;i++)
    {
      short_delay_change = XYZ[i].short_smooth -XYZ[i].hist_smooth_value;
      WriteOutFile(short_delay_change, " ", outwaveinfo);
    }
    fputs("] short-bs[",outwaveinfo);
    for(i=0;i<3;i++)
    {
      short_delay_change = XYZ[i].short_smooth -XYZ[i].base_line;
      WriteOutFile(short_delay_change, " ", outwaveinfo);
    }
    fputs("] long-delay[",outwaveinfo);
    for(i=0;i<3;i++)
    {
      long_delay_change = XYZ[i].smooth_value - XYZ[i].hist_smooth_value;
      WriteOutFile(long_delay_change, " ", outwaveinfo);
    }
  }
  */
  fputs("\r\n", outwaveinfo);
}

void Debug_Adaptive_BS()
{
  uint8_t i=0,j=0;
  // update base line
  fputs(sdate,outwaveinfo);
  WriteOutFile(signal.status, " occupied= ", outwaveinfo);
  for(i=0;i<DIV_NUM;i++)
  {
    WriteOutFile(XYZ[i].bs_buf.average_value, " ave ", outwaveinfo);
    WriteOutFile(XYZ[i].base_line, " bs ", outwaveinfo);
    WriteOutFile(XYZ[i].adaptive_base_line, " abs ", outwaveinfo);
    fputs("[",outwaveinfo);
    j = XYZ[i].bs_buf.head;
    while(j != XYZ[i].bs_buf.tail)
    {
      WriteOutFile(XYZ[i].bs_buf.samples[j], " ", outwaveinfo);
      j++;
      if(j >= BS_BUFFER_LEN)
      {
        j=0;
      }
    }
    fputs("]",outwaveinfo);
  }
  fputs("\r\n", outwaveinfo);
}
#endif

/*********************************************************************
*********************************************************************/
