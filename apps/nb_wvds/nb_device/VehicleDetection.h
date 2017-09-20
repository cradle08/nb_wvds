/**************************************************************************************************
  Filename:       VehicleDetection.h
  Created:        $Date: 2011-11-23 20:34:23 $
  Revised:        $Date: 2012-01-02 19:38:23 $
  Revision:       $Revision: 5 $

  Description:    This file contains the Vehicle Detection definitions.
**************************************************************************************************/

#ifndef VEHICLEDETECTION_H
#define VEHICLEDETECTION_H

/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>
//#include "contiki.h"
//#include "mmc3316.h"
//#include "hmc5983.h"
#include "app.h"

/*********************************************************************
 * CONSTANTS
 */

#define HAS_HMC5983  0
#define HAS_QMC5883  1

/* AUTO_RE_INIT˵��
 * 1��ʾ������Զ��궨
 * 0��ʾ���Զ��궨���������������ֶ��궨
*/
#define AUTO_RE_INIT 0

/* SERIAL_SPACE
 * 1��ʾ���г�λ��ʯ�ý֣��Ƽ���10�����Ǵ��г�λ
 * 0��ʾ�Ǵ��г�λ
 */
#define SERIAL_SPACE    0

/* PARALLEL_SPACE˵��
 * 1��ʾ���г�λ�������������ǲ��г�λ��
 * 0��ʾ���г�λ��ʯ�ý��Ǵ��г�λ
 */
#define PARALLEL_SPACE  1

//#define DEBUG_OUT 0
#define UP_HILLS 1
//#define SIM
#if DEBUG_OUT
#define CLOCK_SECOND  1000
#endif
#ifndef WRITE_ALGO_FLASH
#define WRITE_ALGO_FLASH    1    //״̬дflash,�Է��ڵ��������������³�ʼ��
#endif
#define ACTIVATE_RF_BY_AMR  1  //ͨ��AMR������������Ƶ
#define UNIT_MILLI_GAUSS 0    //������ֵ��λͳһΪ����˹
#define UPLOAD_LEN 10
#define LEAST_HILL_COUNT 3
#define SENSOR_PORT 1
#ifndef IS_SNIFFER_VERSION
#define IS_SNIFFER_VERSION 0 // 1 is the version used for sample magnetic signal
                             // 0 is the version used for formal deployment
#endif
#if SUBWAY_SAPCE==1
#define STABLE_TH   30//25 // the lease threshold, the more sensitive, initial value is 10
#else
#define STABLE_TH   25 // the lease threshold, the more sensitive, initial value is 10
#endif
#define FAST_STABLE_TH 5 // must select different value for subway environment
#define UNOCCUPIED_TH 15
#define ADAPTIVE_THRSH                  // define to use adaptive threshold mechanism
#define DIV_NUM     3
#define HILL_LENGTH 20

#if PARALLEL_SPACE==1
#define INIT_SAMPLE_PERIOD    1120
#define STABLE_SAMPLE_PERIOD  560//560//560 //CLOCK_SECOND/2// //ms
#define DISTURB_SAMPLE_PERIOD 96//96 //192 CLOCK_SECOND/5// //ms
#elif SERIAL_SPACE==1
#define STABLE_SAMPLE_PERIOD  1120//560//560 //CLOCK_SECOND/2// //ms
#define DISTURB_SAMPLE_PERIOD 192//96 //192 CLOCK_SECOND/5// //ms
#endif

#define PARKINGAPP_SEND_PERIODIC_MSG_TIMEOUT CLOCK_SECOND //560
#if PARALLEL_SPACE==1
#define DEFAULT_GAIN          3  //the default gain of hmc5983
#else
#define DEFAULT_GAIN          2  //the default gain of hmc5983
#endif

// Parking Algorithm
#define SINGLE_THRSH        50//45   // when one of x, y, and z over the SINGLE_THRSH, a car detected
#define DOUBLE_THRSH        40//30
#define THIRD_THRSH         30

//thunder[0]=TRUE and possible there has a car parking
#define SINGLE_THRSH_1      20//45
#define DOUBLE_THRSH_1      15//30
#define THIRD_THRSH_1       15

//thunder[0]=FALSE and use these thresholds to detect parking
#define SINGLE_THRSH_B      80   // possible there has a car parking
#define DOUBLE_THRSH_B      50   //30
#define THIRD_THRSH_B       35

#define THUNDER_ONE         15//30
#define THUNDER_DOUBLE      10//15//20
#define UP_THUNDER_ONE      30
#define UP_THUNDER_DOUBLE   20
#define REFER_THRSH         10
#define UP_SINGLE_THRSH     60   // the up threshold for SINGLE_THRSH
#define UP_DOUBLE_THRSH     40    // the up threshold for DOUBLE_THRSH
#define SMOOTH_LENGTH       31    //8 // the pre-defined running average buffer size
#define BASE_WAIT_COUNT     2    //8 // the pre-defined running average buffer size
#define NOCAR_LENGTH        5
#define SMOOTH_PARAM        10   // 8 // the forgetting factor
#define SMOOTH_SUM          100   // the total factor
#define CONFIRM_CHANGE_NUM  5     // over threshold successively than CONFIRM_CHANGE_NUM to confirm the status
#define IGNORE_NUM          10
#define IGNORE_THRESH       10//5    // the theshold to decide the stable state
#define BS_CHANGE_RANGE     30    // the change range of baseline is between -20 - 20
#define REFER_BS_COUNT      500   // the refer base_line is determined after number of REFER_BS_COUNT change
#define BS_SMOOTH_PARA      2

#define BASELINE_THRSH      10    // ignore the value over BASELINE_THRSH for base line update
#define BASELINE_THRSH_2    15
#define SIGNAL_MAX          1200  // the max signal is SIGNAL_MAX in the parking place
#define NORMAL_MAX          1200   // the max signal is NORMAL_MAX in the normal (non-interfere) status
#define THUNDER_THRSH       50   //
#define UP_THUNDER_THRSH    60
#define THUNDER_BIG         70 //90
#define THUNDER_INTER       25
#define ERROR_THRESH        50
#define NEW_CHANGE_THRESH   45//30
#define RESET_THUNDER_COUNT 25 //�ȴ��Ĵ�����signal.is_thunder[]����
#define CAR_ARRIVAL_COUNT   5


#if AUTO_RE_INIT==1
#define DROP_NUM            5
#else
#define DROP_NUM            10
#endif

#if SERIAL_SPACE==1
#define CAR_LEAVE_COUNT     8
#else
#define CAR_LEAVE_COUNT     5
#endif

#define FUSE_LEAVE_COUNT    5
#define CAR_CHANGE_THRSH    10
#define CAR_CHANGE_BIG_THRSH    30

#define CAR_CHANGE_BIG_1    100
#define CAR_CHANGE_BIG_2    45
#define CAR_CHANGE_LITTE_1  5
#define CAR_CHANGE_LITTE_2  8
#define CAR_CHANGE_LITTE_THRSH  20
#define MATCH_RATE          15
#define CAR_CHANGE_RE_RATE  10  //10 percents

#define CHANGE_NUM 7
#if SERIAL_SPACE==1
#define SHORT_SMOOTH 6 // 5
#elif PARALLEL_SPACE==1
#define SHORT_SMOOTH 5
#endif
#define LEAVING_COUNT 3
#define AX_INDEX  0
#define BS_LOCK_NUM 50 //BS_LOCK_NUM > SMOOTH_LENGTH

#define BASE_SMOOTH_1       40
#define BASE_SMOOTH_2       50
#define BS_RECORD_LEN       80
#define AVERAGE_OVER        8
#define AVERAGE_BELOW       10//30 too big will can't detect the short time next car parking
#define AVERAGE_ARRIVAL     5
#define AVERAGE_LEAVE       10
#define AVERAGE_THRESH      60
#define LONG_STABLE         100//50

/***
*��ֵ�������ȵ�У��
*   �ٶ����趨����ֵTΪ������g=390(gain)�µ�ֵ
*   ����������g1�µ���ֵΪT1=T*g1/g
*   ���¸���5%����T1*=95/100���Ȳ�д����ɣ�
****/
//ʻ������ֵ
#if HAS_HMC5983

#if SERIAL_SPACE==1
#define PARKING_THRESHOLD_L   50//60  //150 gain=1 //�ȶ���ͣ����⣬gain=5�µ�little_threshold,�����������Gain������Ҫ�Զ�����
#elif  PARALLEL_SPACE==1
#define PARKING_THRESHOLD_L   45//60  //150 gain=1 //�ȶ���ͣ����⣬gain=5�µ�little_threshold,�����������Gain������Ҫ�Զ�����
#endif

#if SERIAL_SPACE==1
#define PARKING_THRESHOLD_B   90//100 //200 gain=1 //�ȶ���ͣ����⣬gain=5�µ�big_threshold,�����������Gain������Ҫ�Զ�����
#elif  PARALLEL_SPACE==1
#define PARKING_THRESHOLD_B   80//100 //200 gain=1 //�ȶ���ͣ����⣬gain=5�µ�big_threshold,�����������Gain������Ҫ�Զ�����
#endif

#define PARKING_THRESHOLD_F   150 //300 gain=1 //���ټ�⣬gain=5��


//QMC���ܵ��ų��ĸı���ֻ��HMC��0.8����
#elif HAS_QMC5883

#if SERIAL_SPACE==1
#define PARKING_THRESHOLD_L   70   //60  //150 gain=1 //�ȶ���ͣ����⣬gain=5�µ�little_threshold,�����������Gain������Ҫ�Զ�����
#define PARKING_THRESHOLD_B   80
#define UNOCC_THRSHOLD        50  //ʻ���⣬gain=5�µģ������ź��ȶ����˳�����ֵ
#define PARKING_THRESHOLD_F   150
#elif PARALLEL_SPACE==1
#define PARKING_THRESHOLD_L   75  //60  //150 gain=1 //�ȶ���ͣ����⣬gain=5�µ�little_threshold,�����������Gain������Ҫ�Զ�����
#define PARKING_THRESHOLD_B   120 //90
#define UNOCC_THRSHOLD        50  //ʻ���⣬gain=5�µģ������ź��ȶ����˳�����ֵ
#define PARKING_THRESHOLD_F   180
#endif

#endif

//ʻ������ֵ

#define FAST_LEAVING_THRSH    35  //ʻ���⣬gain=5�µĿ����˳�����ֵ
#define CAR_CHANGE_RATE       25  //simil(enter_drift,leave_drift),0<drift<100
#define CAR_CHANGE_RATE_1     40  //only for mmc3316,drift>1500,need reset
#define CAR_CHANGE_RATE_2     30  //simil(enter_drift,leave_drift),drift>100

#define SLOPE_THRESH          15  //��ֵ��ɸѡ��ֻ��ǰ��б��>SLOPE_THRESH,�����Ǽ�ֵ��
#define BIG_SLOPE_THRESH      30  //ͳ�Ʋ�������ľ��ҳ̶�
#define OVER_THUNDER_COUNT    6   //��������Ĵ���
#define OVER_THUNDER_BIG      3   //�ϴ����>BIG_SLOPE_THRESH�Ĵ���
#define MAX_MIN_THRESH        180 //the threshold of (max - min)
#define NUM_THRESH            2   // 5���ж����弰����ʱ��б�ʴ��ڻ�С��0�Ĵ���
#define STRONG_MAG_THRESH     2500 //ǿ���ж�����ֵ
#define MAX_SIGNAL_MAG        1500
#define STRONG_MAG_NUM        3   // big mag num for check big mag
#define WEAK_MAG_NUM          3   // weak mag num for check big mag
   
#define BS_BUFFER_LEN 6
#define UPDATE_BS_T   5
#define BS_WAIT_NUM   100
#define INIT_DEPLOYMENT_NUM 10//sniffer 5; deployment 10
#define INIT_BASELINE_NUM   20//sniffer 10; deployment 20
#define INIT_THRESHOLD_NUM  30//sniffer 15; deployment 30

#define THUNDER_INTER_TIMEOUT_COUNT  200    // Every 8 seconds
#define START_PARKING       5  // ignore number of START_PARKING sample when the node restart
                                  // setting START_PARKING=10 in the test situation
                                  // setting START_PARKING=500 in the parking

#if (defined BS_DEBUG) && (BS_DEBUG == TRUE)
typedef union {
  struct {
    uint8_t thunder0:1;
    uint8_t thunder1:1;
    uint8_t updateLock:1;
    uint8_t thresMatch:1;
    uint8_t reseted:1;
    uint8_t car_change:1;
    uint8_t new_change:1;
    uint8_t reserved:1;
  } data;
  uint8_t _flat;
} flag_value_t;
#endif

#if UP_HILLS==1
typedef struct {
  //uint8_t op; //0x24
  //uint32_t seqno;
  //uint8_t threshold;
 int16_t upload_hill[UPLOAD_LEN][DIV_NUM]; // ������ֵ���� the hill and valley values of a fluctuation
 int16_t upload_bs[DIV_NUM]; // �궨ֵ the adaptive base line
 int16_t upload_smooth[DIV_NUM]; // ��ǰֵ the smooth value of current sample
 uint8_t judge_branch; // ����ʻ���ʻ��ķ�֧
 uint8_t status;
} upload_wave;
#endif

/*
// ����㷨����(Algorithm Paramters)  ,define at app.h,move to vehicledetection.h
struct ALGO {
  uint16_t magic;
  uint16_t normalT;         // ƽ�Ȳ������ڣ��Ժ���Ϊ��λ
  uint16_t flunctT;         // �����������ڣ��Ժ���Ϊ��λ
  uint8_t big_occ_thresh;   // �����г��о���ֵ
  uint8_t mid_occ_thresh;   // ƽ�Ⱥ��г��о���ֵ
  uint8_t litt_occ_thresh;  // �ʹų��о���ֵ
  uint8_t unocc_thresh;     // �޳��о���ֵ
  uint8_t axis_stable_threshold;  // xyz stable thresh ...
//  uint8_t gain_hmc5983;     // �Ŵ���������
  uint8_t status;           // ��λ״̬
  int16_t base_line[3];     // �����ų�����ֵ
  uint16_t crc;             // ALGO��CRC16У���
};


typedef struct{
  int16_t x;
  int16_t y;
  int16_t z;
} Sample_Struct;

extern Sample_Struct One_Sample;
*/

typedef struct {
  int32_t sum;
  int32_t short_sum;
  int32_t samples[SMOOTH_LENGTH];
  uint8_t head;
  uint8_t tail;
  uint8_t len; // the length of valid value
  int32_t hist;
} smooth_buf_t;

typedef struct {
  int16_t average_value;
  int32_t sum;
  int32_t samples[BS_BUFFER_LEN];
  uint8_t head;
  uint8_t tail;
  uint8_t len; // the length of valid value
} bs_buf_t;

enum {
  EN_LEAVE =0,
  EN_PARK = 1,
  EN_BIG_MAG = 2,
  EN_INIT = 3,
  EN_INIT_FAILL = 4
}; // ...

typedef struct {
  /*status
   *parking space status
   *0 is vacant
   *1 is occupance
   *2 ǿ���ź�
   *3 ��ʼ��
   
   */
  uint8_t status;
  uint8_t st_before_fluct;//����ǰ��״̬

  /*stable
   *TRUE signal is stable
   *FALSE signal is fluctuate
   */
  bool stable;
  bool latest_stable;// the latest value of stable
  uint8_t continue_sample_num;// the continues number of sample after from unstable changed to stable
  bool new_change_flag;
  uint16_t long_stable_num;
  bool compute_long_bs;
  uint8_t adaptive_bs_wait_num; // when adaptive_bs_wait_num==1, update baseline
  uint16_t stable_sample_freq;
  uint16_t disturb_sample_freq;

  /*case 1: initialize_flag==0
   *when a node is first deplyment and power on, then enter the initialize phase
   *in the phase, vehicles don't disturb the magnetic signal
   *the node computes the characteristic of environmental magnetic signal
   *and initialize thresholds
   *
   *case 2: initialize_flag==1
   *the node is quite normal,doesn't need enter the initialize phase
   *going detection phase
   *
   *case 3: initialize_flag==2
   *the node is restart,
   *read flash NV_ALGO_ADDR struct ALGO
   *call Set_Algorithm_Parameters(&algo)
   */
  uint8_t initialize_flag;

  //���±궨
  bool re_initial_flag;

  //big_threshold���ڿ��ٳ������
  uint16_t occ_big_threshold;

  //mid_threshold���ڳ���ͣ�Ⱥ�thunder[0]=0,���
  uint16_t occ_mid_threshold;

  //little_threshold���ڳ���ͣ�Ⱥ�thunder[0]=1,���
  uint16_t occ_little_threshold;

  uint8_t unocc_threshold; // to judge the parking space is unoccupied
  uint8_t axis_stable_threshold; // xyz data stable thresh ...

  /*
  the VD is invoked by a constant magnetic signal
  */
  bool invoke_flag;

  bool hist_is_strong_mag;
  bool is_strong_mag; // ...

   /*
  �ȴ�ǿ���뿪
  */
  bool wait_flag;

   /*
  *RF�����ʶ
   *0 ��ʾδ����
   *1 ��ʾ�Ѽ���
   *2 ��ʾ������
  */
  uint8_t activate_rf;

  uint8_t num_count;
  uint8_t rf_num_count;
  uint8_t strong_num_count;
  uint8_t weak_num_count;
  uint8_t init_num_count;


  /*
   * is_thunder[0] is car come thunder,
  * is_thunder[1] is car leave thunder
  */
  bool is_thunder[2];
  uint8_t fuse_status;
  uint8_t h_below_count;
  uint8_t over_count;
  uint8_t p_over_count;
  uint8_t below_count;

  uint8_t b_status;
  bool unfluct_parking;
  uint8_t p_status;
  bool thunder_count_flag;
  uint8_t thunder_count;
  //uint8_t hill_count;
  //uint8_t big_amplitute;
} compositive_t;

typedef struct {
  uint8_t axis_status;         // detection result of this dimension
  bool  axis_stable;         // whether in stable
  int16_t raw_value;      // converted magnet sample
  int16_t base_line;
  int16_t smooth_value;
  int16_t short_smooth;
  int16_t hist_short_smooth;
  int16_t delay_smooth;
  int16_t long_stable_value; // to record the long stable value after a car parking

  //Thresholds
  int16_t stable_thresh; // the thresh is to judge the wave if fluctuation

  uint8_t qi_count; // to num the singularity magnetic samples
  uint8_t wen_count; // to num the stable magnetic samples
  uint8_t unocc_count; // to count the number of the value least than unoccupied_thresh

  // hill and valley
  int16_t hill_valley[HILL_LENGTH];
  uint8_t hill_point; // the head point of hill_valley[]

  //variants to catch hill_valley
  uint8_t big_num;
  int16_t temp_hill;
  uint8_t little_num;

  uint8_t big_amplitute; // the number of the slope who is over the BIG_SLOPE_THRESH
  uint8_t hill_count; // the number of hill and valley in a fluctuation
  uint16_t max_min_amplitute; // the distance of max value and min value in a fluctuation

  /* changes */
  int16_t changes[CHANGE_NUM];
  /* history */
  int16_t hist_raw_value; // lastest history raw sample value

  // hist_base_line[0] restore the base_line before signal flucture
  // hist_base_line[1] restore the history base_line
  // hist_base_line[2] restore the initial base_line
  int16_t setup_base_line;
  int16_t adaptive_base_line;

  int16_t hist_smooth_value;
  /* others */
  uint8_t amr_comp[2];       // AMR temperature compensation scale
  smooth_buf_t smooth;
  bs_buf_t bs_buf;
} cardet_axis_t;


enum {
  CAR_ENTER = 0,
  CAR_LEAVE = 1,
  BASE_ENTER = 2,
  BASE_LEAVE = 3,
  LONG_LEAVE=4,
  NEW_CAR_ENTER = 5,
  NEW_CAR_LEAVE = 6,
  BASE_LINE = 7,
  SMOOTH_VALUE = 8,
  ENTER_BRANCH = 9,
  LEAVE_BRANCH =10,
  SMOOTH_BS =11,
  ADAPTIVE_BS =12,
  SETUP_BS =13
};

/*
enum {
  CAR_ENTER = 0,
  CAR_LEAVE = 1,
  NEW_CAR_ENTER = 2,
  NEW_CAR_LEAVE = 3,
  BASE_LINE = 4,
  SMOOTH_VALUE = 5,
  ENTER_BRANCH = 6,
  LEAVE_BRANCH =7
};
*/
#if defined ( NV_INIT )
typedef struct
{
  bool first_deployment_flag;
  bool latest_statue;
  int16_t init_base_line[3];
  int16_t latest_base_line[3];
} NVapp;
#endif

/*********************************************************************
* LOCAL FUNCTIONS
*/
bool Space_Status(uint16_t a_value, uint16_t b_value, uint8_t CURR_DOUBLE_THRSH, uint8_t CURR_THIRD_THRSH);
void Raw_Signal_Smooth(cardet_axis_t* axis);
void Base_Line_Smooth(void);
void Raw_Data_Smooth(void);
void Set_Raw_Value(void);
void Adaptive_Sampling(void);
uint8_t Is_Over_Thresh(uint8_t CURR_SINGLE_THRSH, uint8_t CURR_DOUBLE_THRSH, uint8_t CURR_THIRD_THRSH);
void Compute_Change(void);
void Car_Leaved_Init(void);
uint8_t Is_Car_Leaved(uint8_t ca, uint8_t cb);
uint8_t Is_Below_Thresh(uint8_t one_thresh, int8_t bs_branch);
void Deal_Arrival_Error(void);
void Set_New_Change(void);
bool Is_All_Zero(int16_t a_array[]);
void Reset_Base_Line(void);
bool Over_Main_Thresh(uint16_t t0, uint16_t t1, uint16_t t2, uint8_t main_thresh);
void Car_Leave_Functions(uint8_t leave_num);
void Car_Arrival_Functions(uint8_t arrival_num);
uint8_t Is_Base_Car_Leaved(uint8_t ca, uint8_t cb);
void XYZ_IS_Fluctuation(void);
void XYZ_IS_Stable(void);
void Signal_IS_Stable(void);
void Set_Hill_Valley(void);
void IS_Parking(void);
void IS_Leaving(void);
void Set_His_Value(void);
void Reset_Is_Thunder(void);
void Set_Before_Smooth(void);
void Set_Before_Hill(void);
void Set_After_Hill(void);
void Calculate_Amplitude(void);
void Check_Hill_Amplitude(void);
void Set_Before_Fluctuate(void);
void Set_After_Fluctuate(void);
bool Is_Strong_Magnetic(void);
//int16_t Compute_Number(int16_t c_num, uint8_t a_num, uint8_t b_num);
int16_t Compute_Number(int16_t x1, int16_t x2, uint8_t a_num, uint8_t b_num);
// function for quick response
void Fast_Arrival_Response(void);
void Fast_Leaving_Response(void);
bool Base_Smooth_Check(void);
void Unfluctuate_Parking(void);
static bool changes_is_all_zero(uint8_t cId);
uint8_t Average_Over_Thresh(uint8_t a_thresh);
uint8_t Smooth_Base_Status(uint16_t a_thresh);
//static bool base_is_all_zero(void);
uint8_t Entering_Drift(void);
void Init_process(void);
void Statistic_Wave_Shape(void);
void Record_Long_Stable(void);
uint8_t Is_Change_Sim(uint8_t ca, uint8_t cb);
void Adaptive_base_line(void);
void Fill_base_buf(bs_buf_t* bs_buf, int16_t current_value);
void Update_base_line(cardet_axis_t* axis);//bs_buf_t* bs_buf, smooth_buf_t* smooth);
void Reset_After_Strong_Mag(void);

bool check_constant_signal(uint8_t activate_branch);
uint8_t wait_and_reset(void);
void Set_AMR_Period(int period); // reset sample time ...
uint16_t Get_AMR_Period(); //
void Re_Init_Request(); // re init algorithm ...
int Set_Algorithm_Parameters(struct ALGO * paras);
void Get_Algorithm_Parameters(struct ALGO * paras);
void Variant_Init();

#define car_leave_change_is_all_zero() changes_is_all_zero(CAR_LEAVE)
#define car_enter_change_is_all_zero() changes_is_all_zero(CAR_ENTER)
#define new_leave_change_is_all_zero() changes_is_all_zero(NEW_CAR_LEAVE)
#define new_enter_change_is_all_zero() changes_is_all_zero(NEW_CAR_ENTER)
#define base_enter_change_is_all_zero() changes_is_all_zero(BASE_ENTER)
#define base_leave_change_is_all_zero() changes_is_all_zero(BASE_LEAVE)
#define long_leave_change_is_all_zero() changes_is_all_zero(LONG_LEAVE)



#endif /* VEHICLEDETECTION_H */
