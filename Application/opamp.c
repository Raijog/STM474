#include "opamp.h"
#include "types.h"
#include "stdlib.h"
#include "main.h" // Todo: muss hier noch raus, wenn Toggle Timer3- out entfernt ist

#define DMA_BUFFER     50     	  // Länge des DMA-Buffers
#define ABTAST_FREQ    44100    	// Abtastfrequenz 44,1 kHz
#define LOW_PASS_FREQ  1          // fg = 1Hz
#define FIX_COMMA_POS  20         // binäre Nachkommastellen
#define FIX_COMMA_VAL  1048576    // Multiplikator fuer Fixkommanberechnung (2^FIC_COMMA_POS)
//#define MIDVALUE_KOEFF_A (((1 / (ABTAST_FREQ/2/3.14/MIDVALUE_FREQ + 1)) * FIX_COMMA_MULT))
//#define MIDVALUE_KOEFF_B (FIX_COMMA_MULT - MIDVALUE_KOEFF_A)
#define KOEFF_A ((int32_t)(1/(ABTAST_FREQ/2/3.14159/LOW_PASS_FREQ + 1)*FIX_COMMA_VAL))
#define KOEFF_B ((int64_t)(FIX_COMMA_VAL - KOEFF_A))

void calcAmplitude(void);


static int32_t opampGainList[][2] = {{ 2, OPAMP_PGA_GAIN_2_OR_MINUS_1},
																		 { 4, OPAMP_PGA_GAIN_4_OR_MINUS_3},
																		 { 8, OPAMP_PGA_GAIN_8_OR_MINUS_7},
																		 {16, OPAMP_PGA_GAIN_16_OR_MINUS_15},
																		 {32, OPAMP_PGA_GAIN_32_OR_MINUS_31},
																		 {64, OPAMP_PGA_GAIN_64_OR_MINUS_63}
																		};
extern uint16_t value_adc[];

static uint16_t adcRangeBegin;
static uint8_t ucRise = FALSE;
static uint16_t lastValue = 0;
uint16_t maxValue = 0;
uint16_t actMaxValue = 0;
uint16_t avgMaxValue = 0;
volatile float midValue = 0;
float dAvgMaxValue = 0;
int32_t lAvgValue = 0;
int32_t lAvgMaxValue = 0;
int16_t iAvgValue = 0;																		
																		
void opampHandle(OPAMP_HandleTypeDef *hopamp)
{
	int32_t dummy = opampGainList[0][1];
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
   adcRangeBegin = 0;
	 calcAmplitude();
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	 adcRangeBegin = DMA_BUFFER/2;
   calcAmplitude();
}

void calcAmplitude(void)
{
  // Die Funktion benötigt für 25 Werte 24 us)
	int8_t i;
	
	
	HAL_GPIO_WritePin(GPIOC,Tim3_Out_Pin,GPIO_PIN_SET);
	
	for(i=0;i<DMA_BUFFER/2;i++)
   {
		 //midValue = MIDVALUE_KOEFF_A * (float)value_adc[adcRangeBegin + i] + MIDVALUE_KOEFF_B * midValue;
		 
		 lAvgValue = KOEFF_A * value_adc[adcRangeBegin + i] + (int32_t)((KOEFF_B * lAvgValue + FIX_COMMA_VAL/2) >> FIX_COMMA_POS);
		 
   }
	 
	 iAvgValue = (int16_t)(lAvgValue >> FIX_COMMA_POS);
	 
	 
	 for(i=0;i<DMA_BUFFER/2;i++)
	 {
			
			if (lastValue < iAvgValue && value_adc[adcRangeBegin +i] > iAvgValue)
			{
				ucRise = TRUE;
				actMaxValue = value_adc[adcRangeBegin + i];
				//lastValue = value_adc[adcRangeBegin +i];
			}
			else if (lastValue > iAvgValue && value_adc[adcRangeBegin +i] < iAvgValue)
			{
				ucRise = FALSE;
				maxValue = actMaxValue;
				//lastValue = value_adc[adcRangeBegin +i];
			}
		 
			if (abs((int16_t)lastValue - value_adc[adcRangeBegin +i]) > 2)
			{
				lastValue = value_adc[adcRangeBegin +i];
			}				
		 
			if (ucRise && value_adc[adcRangeBegin +i] > actMaxValue)
			{
				actMaxValue = value_adc[adcRangeBegin +i];
			}
			//dAvgMaxValue = MIDVALUE_KOEFF_A * maxValue + MIDVALUE_KOEFF_B * dAvgMaxValue;
			lAvgMaxValue = KOEFF_A * actMaxValue + (int32_t)((KOEFF_B * lAvgMaxValue + FIX_COMMA_VAL/2) >> FIX_COMMA_POS);
			
   }
	 avgMaxValue = (int16_t)((lAvgMaxValue + FIX_COMMA_VAL/2) >> FIX_COMMA_POS);
	 
	 //ToDo: Ohne Eingangssignal geht maxValue nicht mehr nach 0. Die Auswertung sollte einen neuen MAX-Wert liefern, wenn die Kurver oberhalb des Mittelwertes ist.
	 //Wenn aber der Mittelwert 0 ist ändert sich am letzten MAx-WErt nichts mehr. 
	 
	 HAL_GPIO_WritePin(GPIOC,Tim3_Out_Pin,GPIO_PIN_RESET);
	
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim3)
{
	 //todo HAL_GPIO_TogglePin(GPIOC,Tim3_Out_Pin);
}
