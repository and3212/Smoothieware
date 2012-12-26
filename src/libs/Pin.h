#ifndef PIN_H
#define PIN_H

#include <stdlib.h>
#include <stdio.h>
#include "libs/LPC17xx/sLPC17xx.h" // smoothed mbed.h lib
#include "libs/Kernel.h"
#include "libs/utils.h"
#include <string>

class Pin {
    public:
        Pin(){
            _pwm = -1;
            _sd_accumulator = 0;
        }

        Pin* from_string(std::string value){
            LPC_GPIO_TypeDef* gpios[5] ={LPC_GPIO0,LPC_GPIO1,LPC_GPIO2,LPC_GPIO3,LPC_GPIO4};
            if( value.find_first_of("n")!=string::npos ? true : false ){
                this->port_number = 0;
                this->port = gpios[(unsigned int) this->port_number];
                this->inverting = false;
                this->pin = 255;
            }else{
                this->port_number =  atoi(value.substr(0,1).c_str());
                this->port = gpios[(unsigned int) this->port_number];
                this->inverting = ( value.find_first_of("!")!=string::npos ? true : false );
                this->pin  = atoi( value.substr(2, value.size()-2-(this->inverting?1:0)).c_str() );
                this->port->FIOMASK &= ~(1 << this->pin);
            }
            _pwm = -1;
            _sd_accumulator = 0;
            return this;
        }

        inline bool connected(){
            if( this->pin == 255 ){ return false; }
            return true;
        }

        inline Pin* as_output(){
            if (this->pin == 255) return this;
            this->port->FIODIR |= 1<<this->pin;
            return this;
        }

        inline Pin* as_input(){
            if (this->pin == 255) return this;
            this->port->FIODIR &= ~(1<<this->pin);
            return this;
        }

        inline Pin* as_open_drain(){
            if (this->pin == 255) return this;
            if( this->port_number == 0 ){ LPC_PINCON->PINMODE_OD0 |= (1<<this->pin); }
            if( this->port_number == 1 ){ LPC_PINCON->PINMODE_OD1 |= (1<<this->pin); }
            if( this->port_number == 2 ){ LPC_PINCON->PINMODE_OD2 |= (1<<this->pin); }
            if( this->port_number == 3 ){ LPC_PINCON->PINMODE_OD3 |= (1<<this->pin); }
            if( this->port_number == 4 ){ LPC_PINCON->PINMODE_OD4 |= (1<<this->pin); }
            return this;
        }

        inline Pin* pull_up(){
            if (this->pin == 255) return this;
            // Set the two bits for this pin as 00
            if( this->port_number == 0 && this->pin < 16  ){ LPC_PINCON->PINMODE0 &= ~(3<<( this->pin    *2)); }
            if( this->port_number == 0 && this->pin >= 16 ){ LPC_PINCON->PINMODE1 &= ~(3<<((this->pin-16)*2)); }
            if( this->port_number == 1 && this->pin < 16  ){ LPC_PINCON->PINMODE2 &= ~(3<<( this->pin    *2)); }
            if( this->port_number == 1 && this->pin >= 16 ){ LPC_PINCON->PINMODE3 &= ~(3<<((this->pin-16)*2)); }
            if( this->port_number == 2 && this->pin < 16  ){ LPC_PINCON->PINMODE4 &= ~(3<<( this->pin    *2)); }
            if( this->port_number == 3 && this->pin >= 16 ){ LPC_PINCON->PINMODE7 &= ~(3<<((this->pin-16)*2)); }
            if( this->port_number == 4 && this->pin >= 16 ){ LPC_PINCON->PINMODE9 &= ~(3<<((this->pin-16)*2)); }
            return this;
        }

        inline Pin* pull_down(){
            if (this->pin == 255) return this;
            // Set the two bits for this pin as 11
            if( this->port_number == 0 && this->pin < 16  ){ LPC_PINCON->PINMODE0 |= (3<<( this->pin    *2)); }
            if( this->port_number == 0 && this->pin >= 16 ){ LPC_PINCON->PINMODE1 |= (3<<((this->pin-16)*2)); }
            if( this->port_number == 1 && this->pin < 16  ){ LPC_PINCON->PINMODE2 |= (3<<( this->pin    *2)); }
            if( this->port_number == 1 && this->pin >= 16 ){ LPC_PINCON->PINMODE3 |= (3<<((this->pin-16)*2)); }
            if( this->port_number == 2 && this->pin < 16  ){ LPC_PINCON->PINMODE4 |= (3<<( this->pin    *2)); }
            if( this->port_number == 3 && this->pin >= 16 ){ LPC_PINCON->PINMODE7 |= (3<<((this->pin-16)*2)); }
            if( this->port_number == 4 && this->pin >= 16 ){ LPC_PINCON->PINMODE9 |= (3<<((this->pin-16)*2)); }
            return this;
        }

        inline bool get(){
            if (this->pin == 255) return false;
            return this->inverting ^ (( this->port->FIOPIN >> this->pin ) & 1);
        }

        inline void set(bool value)
        {
            if (this->pin == 255) return;
            _pwm = -1;
            _set(value);
        }

        inline void _set(bool value)
        {
            if ( this->inverting ^ value )
                this->port->FIOSET = 1 << this->pin;
            else
                this->port->FIOCLR = 1 << this->pin;
        }

        inline void pwm(int value)
        {
            if (value > 1023)
                value = 1023;
            if (value < 0)
                value = 0;
            _pwm = value;
        }

        // SIGMA-DELTA modulator
        uint32_t tick(uint32_t dummy)
        {
            if ((_pwm < 0) || (_pwm > 1023))
                return dummy;
            if (_sd_accumulator < -1024)
                _sd_accumulator = -1024;
            if (_sd_accumulator > 2048)
                _sd_accumulator = 2048;
//             printf("[%d %d]", _pwm, _sd_accumulator);
            if (_sd_direction == false)
            {
                _sd_accumulator += _pwm;
                if (_sd_accumulator >= (1024 >> 1))
                    _sd_direction = true;
            }
            else
            {
                _sd_accumulator -= (1024 - _pwm);
                if (_sd_accumulator <= 0)
                    _sd_direction = false;
            }
            _set(_sd_direction);

            return dummy;
        }

        bool inverting;
        LPC_GPIO_TypeDef* port;
        char port_number;
        char pin;
        int _pwm;

        // SIGMA-DELTA pwm counters
        int _sd_accumulator;
        bool _sd_direction;
};




#endif
