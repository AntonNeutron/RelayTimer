/*
  ��������� ������� ��������� �������� (����������� ���� ����������)
  ����������� �� Youtube: https://youtu.be/e0UdgBVcjC4
  ��������� �� GitHub: https://github.com/AntonNeutron/RelayTimer
  �����: AntonNeutron, 2020
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/wdt.h>
volatile uint8_t Flag_tmr, Flag_Zero, Flag_Relay_Start_On,  Flag_Relay_Start_Off, Flag_Relay_Stop_pwr, Flag_Power_On, pre_sumOff;

/*--------------------------------------------*/
#define PIN_RELAY_ON 3
#define PIN_RELAY_OFF 4
#define PIN_LED 0
/*--------------------------------------------*/
#define POWER_ON_AFTER_SEC 3  // ����� ������� ������ ����� ������ ���������� ���� ���������
#define RELE_PULSE	7	// * 10ms   ������� ������ ������� �� ������� ����
#define TIME_OFF_PWR 5  // * 10ms   ���� ������� ������� �� ��������� ����� ���� �������� ��������
/*--------------------------------------------*/

//           ����     76543210		5 ��� ��� ������ ������� � ����� ������ ����� ������� ��� PORTB |= (1<<5);//PORTB &= ~(1<<5);
#define DDRB_MASK 	0b00011001	//��������� �������� ����� x �� ���� ��� �����. 0 � ����� �������� ��� ����. 1 - ����� �������� �� �����.
#define PORTB_MASK 	0b00100101	//���������� ���������� ������ ����� 0 - Hi-Z, 1 - PullUp, ���� ����� 0- ���0, 1- ���1
#define POWER_ON_AFTER_MS POWER_ON_AFTER_SEC

void startON()
{
	
}

int main()
 { 
  //uint8_t ;
  uint16_t count_nofind_sin ,pre_sum10ms, sec_count;
  uint8_t Save_MCUSR;
  Save_MCUSR = MCUSR;
  MCUSR = 0;
  Flag_tmr = Flag_Zero = count_nofind_sin = 0;
  pre_sum10ms = sec_count = 0; 
  Flag_Relay_Start_On = 0; 
  Flag_Relay_Start_Off = 0;
  Flag_Relay_Stop_pwr = 0;
  Flag_Power_On = 0;

  DDRB  = DDRB_MASK;
  PORTB = PORTB_MASK; 
  wdt_disable(); // ���������� ������

  // ��������� ���� ������������ ��������� �� �� ����������� �������, �� �������� ���� 
  if (~Save_MCUSR & (1<<WDRF)) { 
    /* �������� �� �� ����������� �������,����� ���� ���� �������� ��� ������ �������*/
	_delay_ms(3000);  /*�������� 3 ��� �� ������� �������������*/
    Flag_Relay_Start_Off = 1; // ��������� ����� ���������� ����
   /* PORTB |= (1<<PIN_RELAY_OFF);
    _delay_ms(RELE_PULSE * 10);  
    PORTB &= ~(1<<PIN_RELAY_OFF);*/
  }
  /*��� ������ ������ ������� ��� ���� ���������*/
  
  
  
 
   // ---------------------- ��������� ������� �� 10 �� -------------------------------------------------------------------------
   TCCR0A  = TCCR0B = 0;					// ������������� ������
   TCNT0  = 0x44;					// ���������� ������� �������
   OCR0A   = 0xBB;				// ������������� ������� ��������� (�� ���� ����� ������ ������, ����� ����� [����� CTC])
   // ����� ������ ���������� � �������� TIFR ����������� ������� 1 � ��������������� ������. 
   //��� �������� ����� ��������� ������ ����������� ��������, � �� � ������� ���������� ��� !!!!
   TIFR0   = (1<<OCF0A);                   // ������� ���� ���������� ������� �0 �� ��������� 
   TIMSK0  |= (1<<OCIE0A);                  // ��������� ���������� �� ���������
   TCCR0A  = (1<<WGM01);
   TCCR0B  = (0<<CS02)|(1<<CS01)|(1<<CS00);   // ������ ������� ��������� - 64  ����� - ���
   //------------------------------------------------------------------------------------------------------------
   ACSR =  (1<<ACIE)|(1<<ACBG)|(1<<ACIS1)|(1<<ACIS0); // ��������� � ������ �����������
   //------------------------------------------------------------------------------------------------------------
  // WDTCR = 1 << WDTIE; //���������� ���������� �� ����������� �������

   sei(); //��������� ���������� ����������
   wdt_enable(WDTO_30MS); // �������� ������
   while (1){
	if (Flag_tmr){ // ����������� ������ 10��
		Flag_tmr = 0;
/*--------------- ��������� ��������� � ��������� ����------------------------*/

		if (Flag_Zero) { //PORTB |= (1<<2);
		  Flag_Zero = 0;
		  count_nofind_sin = 0;
		  if (!Flag_Power_On) { // ���� ���� ���������
			pre_sum10ms++;
			if (pre_sum10ms%(50 - ((sec_count * 45) / POWER_ON_AFTER_SEC) ) == 0) { // ��� ������� ������� ������� ����������� ��� ����� � ���������, ��� �������
			    PORTB ^=(1<<PIN_LED);
			}
			if (pre_sum10ms == 50) {  // ������ �������� 1 �������
				pre_sum10ms = 0;
				sec_count++;
				if (sec_count == POWER_ON_AFTER_SEC){
					sec_count = 0;
					Flag_Relay_Start_On = 1;
				}
			} // if (pre_sum10ms == 50)
		  } //if (!Flag_Power_On)
		  //PORTB &= ~(1<<2); 	
		} else { // if (Flag_Zero)
			count_nofind_sin++;
			if (count_nofind_sin == TIME_OFF_PWR) {
					PORTB |= (1<<PIN_LED);
					count_nofind_sin = pre_sum10ms = sec_count = 0;
					if (Flag_Power_On)	Flag_Relay_Start_Off = 1;
			}
		} //  else { // if (Flag_Zero)

/*-----------------------------------------------------------------------*/
		if (Flag_Relay_Stop_pwr) { // ��������� ������ ���������� �� ������� ���� ����� ����������� �����
			pre_sumOff++;
			if (pre_sumOff == RELE_PULSE) {
				pre_sumOff = 0;
				Flag_Relay_Stop_pwr = 0;
				PORTB &= ~(1<<PIN_RELAY_OFF);
				PORTB &= ~(1<<PIN_RELAY_ON);
			}
		} //if (FlagOFFRelay) 

		// ���������� ��������, ������������� ����������� ����� ��� ��� ��� ���������� ������� ��� �� ����� ��������� Zero
		if (Flag_Relay_Start_Off){
			PORTB |= (1<<PIN_RELAY_OFF);
			Flag_Relay_Start_Off = 0;
			Flag_Power_On = 0;
			pre_sumOff = 0;
			Flag_Relay_Stop_pwr = 1; // ������� ���� ��� ������� ���� ����� ����������� ����� ����� ���������
			
		}
		wdt_reset(); //����� ����������� �������
	}

};

   return 0;
 }

ISR(TIM0_COMPA_vect){ // ���������� ������� ~10��
  Flag_tmr = 1;
}

ISR(ANA_COMP_vect){ 
// ���������� �����������
	Flag_Zero = 1;
	//PORTB |= (1<<2);
	if (Flag_Relay_Start_On) {
		PORTB |= (1<<PIN_RELAY_ON);
		PORTB &= ~(1<<PIN_LED);
		Flag_Relay_Start_On = 0;
		Flag_Power_On = 1;
		pre_sumOff = 0;
		Flag_Relay_Stop_pwr = 1; // ������� ���� ��� ������� ������� ���� ����� ����������� ����� ����� ���������
	}
    // ����� ���� � �����, ��������� ��� ���������� ���� ���� ��� ���� �������� ��� ������ �������, ��� ���������� ���������� ������ ���������� ����������� �� ���������
	if (Flag_Relay_Start_Off){
			PORTB |= (1<<PIN_RELAY_OFF);
			Flag_Relay_Start_Off = 0;
			Flag_Power_On = 0;
			pre_sumOff = 0;
			Flag_Relay_Stop_pwr = 1; // ������� ���� ��� ������� ���� ����� ����������� ����� ����� ���������
			
	}
	//PORTB &= ~(1<<2);
}