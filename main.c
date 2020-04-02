/*
  Программа таймера включения нагрузки (модификация реле напряжения)
  Потробности на Youtube: https://youtu.be/e0UdgBVcjC4
  Исходники на GitHub: https://github.com/AntonNeutron/RelayTimer
  Автор: AntonNeutron, 2020
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
#define POWER_ON_AFTER_SEC 3  // через сколько секунд после подачи напряжения реле включится
#define RELE_PULSE	7	// * 10ms   сколько длится импульс на обмотку реле
#define TIME_OFF_PWR 5  // * 10ms   если пропадёт питание на указанное время реле отключит нагрузку
/*--------------------------------------------*/

//           биты     76543210		5 бит для тестов перевел в выход теперь можно дергать его PORTB |= (1<<5);//PORTB &= ~(1<<5);
#define DDRB_MASK 	0b00011001	//Настройка разрядов порта x на вход или выход. 0 — вывод работает как ВХОД. 1 - вывод работает на ВЫХОД.
#define PORTB_MASK 	0b00100101	//Управление состоянием входов порта 0 - Hi-Z, 1 - PullUp, если выход 0- лог0, 1- лог1
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
  wdt_disable(); // отключаеем собаку

  // проверяем если перезагрузка произошла не по сторежевому таймеру, то выключам реле 
  if (~Save_MCUSR & (1<<WDRF)) { 
    /* Загрузка НЕ по сторожевому таймеру,Вдруг реле было включено при подаче питания*/
	_delay_ms(3000);  /*Задержка 3 сек на зарядку конденсаторов*/
    Flag_Relay_Start_Off = 1; // установка флага откдючения реле
   /* PORTB |= (1<<PIN_RELAY_OFF);
    _delay_ms(RELE_PULSE * 10);  
    PORTB &= ~(1<<PIN_RELAY_OFF);*/
  }
  /*при старте всегда считаем что реле выключено*/
  
  
  
 
   // ---------------------- Настройка таймера на 10 мс -------------------------------------------------------------------------
   TCCR0A  = TCCR0B = 0;					// останавливаем таймер
   TCNT0  = 0x44;					// сбрасываем счетный регистр
   OCR0A   = 0xBB;				// устанавливаем регистр сравнения (до него будет тикать таймер, потом сброс [режим CTC])
   // Сброс флагов прерываний в регистре TIFR выполняется записью 1 в соответствующий разряд. 
   //Эту операцию нужно выполнять именно перезаписью регистра, а не с помощью побитового ИЛИ !!!!
   TIFR0   = (1<<OCF0A);                   // очищаем флаг прерывания таймера Т0 по сравнению 
   TIMSK0  |= (1<<OCIE0A);                  // разрешаем прерывание по сравнению
   TCCR0A  = (1<<WGM01);
   TCCR0B  = (0<<CS02)|(1<<CS01)|(1<<CS00);   // запуск таймера прескалер - 64  режим - СТС
   //------------------------------------------------------------------------------------------------------------
   ACSR =  (1<<ACIE)|(1<<ACBG)|(1<<ACIS1)|(1<<ACIS0); // Настройка и запуск компаратора
   //------------------------------------------------------------------------------------------------------------
  // WDTCR = 1 << WDTIE; //разрешение прерываний от сторожевого таймера

   sei(); //разрешаем глобальные прерывания
   wdt_enable(WDTO_30MS); // Включаем собаку
   while (1){
	if (Flag_tmr){ // срабатывает каждые 10мс
		Flag_tmr = 0;
/*--------------- Обработка импульсов с детектора нуля------------------------*/

		if (Flag_Zero) { //PORTB |= (1<<2);
		  Flag_Zero = 0;
		  count_nofind_sin = 0;
		  if (!Flag_Power_On) { // если реле выключено
			pre_sum10ms++;
			if (pre_sum10ms%(50 - ((sec_count * 45) / POWER_ON_AFTER_SEC) ) == 0) { // при отсчёте времени моргаем светодиодом чем ближе к включению, тем быстрее
			    PORTB ^=(1<<PIN_LED);
			}
			if (pre_sum10ms == 50) {  // прошло примерно 1 секунда
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
		if (Flag_Relay_Stop_pwr) { // выключаем подачу напряжения на обмотки реле через определённое время
			pre_sumOff++;
			if (pre_sumOff == RELE_PULSE) {
				pre_sumOff = 0;
				Flag_Relay_Stop_pwr = 0;
				PORTB &= ~(1<<PIN_RELAY_OFF);
				PORTB &= ~(1<<PIN_RELAY_ON);
			}
		} //if (FlagOFFRelay) 

		// отключение нагрузки, располагается дублируется здесь так как при отключении питания уже не будет импульсов Zero
		if (Flag_Relay_Start_Off){
			PORTB |= (1<<PIN_RELAY_OFF);
			Flag_Relay_Start_Off = 0;
			Flag_Power_On = 0;
			pre_sumOff = 0;
			Flag_Relay_Stop_pwr = 1; // взводим флаг что питание реле через определённое время нужно отключить
			
		}
		wdt_reset(); //сброс сторожевого таймера
	}

};

   return 0;
 }

ISR(TIM0_COMPA_vect){ // прерывание таймера ~10мс
  Flag_tmr = 1;
}

ISR(ANA_COMP_vect){ 
// прерывание компаратора
	Flag_Zero = 1;
	//PORTB |= (1<<2);
	if (Flag_Relay_Start_On) {
		PORTB |= (1<<PIN_RELAY_ON);
		PORTB &= ~(1<<PIN_LED);
		Flag_Relay_Start_On = 0;
		Flag_Power_On = 1;
		pre_sumOff = 0;
		Flag_Relay_Stop_pwr = 1; // взводим флаг что питание обмотки реле через определённое время нужно отключить
	}
    // дубль кода в цикле, необходим для отключения реле если оно было включено при подаце питания, при пропадении напряжения данное прерывание естественно не сработает
	if (Flag_Relay_Start_Off){
			PORTB |= (1<<PIN_RELAY_OFF);
			Flag_Relay_Start_Off = 0;
			Flag_Power_On = 0;
			pre_sumOff = 0;
			Flag_Relay_Stop_pwr = 1; // взводим флаг что питание реле через определённое время нужно отключить
			
	}
	//PORTB &= ~(1<<2);
}