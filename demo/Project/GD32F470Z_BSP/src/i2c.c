#include "gd32f4xx.h"
#include "i2c.h"
#include <stdio.h>

#define I2C_TIMEOUT 50000

void i2c0_gpio_config(void) {
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_af_set(GPIOB, GPIO_AF_4, GPIO_PIN_8);
    gpio_af_set(GPIOB, GPIO_AF_4, GPIO_PIN_9);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
}

void i2c0_config(void) {
    rcu_periph_clock_enable(RCU_I2C0);
    i2c_clock_config(I2C0, I2C0_SPEED, I2C_DTCY_2);
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C0_SLAVE_ADDRESS7);
    i2c_enable(I2C0);
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);
}

void i2c1_gpio_config(void) {
    rcu_periph_clock_enable(RCU_GPIOF);
    gpio_af_set(GPIOF, GPIO_AF_4, GPIO_PIN_1);
    gpio_af_set(GPIOF, GPIO_AF_4, GPIO_PIN_0);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_1);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
}

void i2c1_config(void) {
    rcu_periph_clock_enable(RCU_I2C1);
    i2c_clock_config(I2C1, I2C1_SPEED, I2C_DTCY_2);
    i2c_mode_addr_config(I2C1, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C1_SLAVE_ADDRESS7);
    i2c_enable(I2C1);
    i2c_ack_config(I2C1, I2C_ACK_ENABLE);
}

void init_i2c(void) {
    i2c0_gpio_config();
    i2c0_config();
    i2c1_gpio_config();
    i2c1_config();
}

void i2c_delay(uint32_t time) {
    for(volatile uint32_t i=0; i<time; i++);
}

uint8_t i2c_addr_poll(uint32_t i2c_periph, uint8_t poll_addr) {
    uint32_t timeout = I2C_TIMEOUT;
    while(i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if(--timeout == 0) return 0;
    }
    
    i2c_start_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if(--timeout == 0) return 0;
    }
    
    i2c_master_addressing(i2c_periph, poll_addr, I2C_TRANSMITTER);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
        if(--timeout == 0) {
            i2c_stop_on_bus(i2c_periph);
            return 0;
        }
    }
    
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
    i2c_stop_on_bus(i2c_periph);
    
    timeout = I2C_TIMEOUT;
    while(I2C_CTL0(i2c_periph) & 0x0200) {
        if(--timeout == 0) return 0;
    }
    return 1;
}

i2c_addr_def get_board_address(uint8_t address) {
    i2c_addr_def eboard_addess = {0, 0, 0};
    if(i2c_addr_poll(I2C0, address)) {
        eboard_addess.periph = I2C0;
        eboard_addess.addr = address;
        eboard_addess.flag = 1;
    } else if(i2c_addr_poll(I2C1, address)) {
        eboard_addess.periph = I2C1;
        eboard_addess.addr = address;
        eboard_addess.flag = 1;
    }
    return eboard_addess;
}

i2c_addr_def scan_board_range(uint8_t start_addr, uint8_t count) {
    i2c_addr_def found = {0, 0, 0};
    for(uint8_t i=0; i<count; i++) {
        found = get_board_address(start_addr + i*2);
        if(found.flag) return found;
    }
    return found;
}

void i2c_cmd_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t cmd) {
    uint32_t timeout = I2C_TIMEOUT;
    while(i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if(--timeout == 0) return;
    }
    
    i2c_start_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
    
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_data_transmit(i2c_periph, cmd);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_stop_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(I2C_CTL0(i2c_periph) & 0x0200) {
        if(--timeout == 0) return;
    }
}

void i2c_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer) {
    uint32_t timeout = I2C_TIMEOUT;
    while(i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if(--timeout == 0) return;
    }
    
    i2c_start_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
    
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_data_transmit(i2c_periph, write_address);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_data_transmit(i2c_periph, buffer);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_stop_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(I2C_CTL0(i2c_periph) & 0x0200) {
        if(--timeout == 0) return;
    }
}

void i2c_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t* p_buffer, uint8_t number_of_byte) {
    uint32_t timeout = I2C_TIMEOUT;
    while(i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if(--timeout == 0) return;
    }
    
    i2c_start_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
    
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    i2c_data_transmit(i2c_periph, write_address);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
    }
    
    while(number_of_byte--) {
        i2c_data_transmit(i2c_periph, *p_buffer++);
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return; }
        }
    }
    
    i2c_stop_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(I2C_CTL0(i2c_periph) & 0x0200) {
        if(--timeout == 0) return;
    }
}

uint8_t i2c_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte) {
    uint32_t timeout = I2C_TIMEOUT;
    
    while(i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if(--timeout == 0) return 0;
    }
    
    i2c_start_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
    }
    
    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
    
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
    }
    
    i2c_data_transmit(i2c_periph, read_address);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
    }
    
    i2c_start_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
    }
    
    i2c_master_addressing(i2c_periph, i2c_addr, I2C_RECEIVER);
    
    if(number_of_byte == 1) {
        i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
        i2c_stop_on_bus(i2c_periph);
    } else if(number_of_byte == 2) {
        i2c_ackpos_config(i2c_periph, I2C_ACKPOS_NEXT);
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
        i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
    } else {
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
        i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
    }
    
    while(number_of_byte) {
        if(number_of_byte == 3) {
            timeout = I2C_TIMEOUT;
            while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
                if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
            }
            i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
        }
        if(number_of_byte == 2) {
            timeout = I2C_TIMEOUT;
            while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
                if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
            }
            i2c_stop_on_bus(i2c_periph);
        }
        
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_RBNE)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        
        *p_buffer++ = i2c_data_receive(i2c_periph);
        number_of_byte--;
    }
    
    timeout = I2C_TIMEOUT;
    while(I2C_CTL0(i2c_periph) & 0x0200) {
        if(--timeout == 0) break;
    }
    
    i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
    i2c_ackpos_config(i2c_periph, I2C_ACKPOS_CURRENT);
    return 1;
}

uint8_t i2c_read_no_reg(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t* p_buffer, uint16_t number_of_byte) {
    uint32_t timeout = I2C_TIMEOUT;
    
    while(i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if(--timeout == 0) return 0;
    }
    
    i2c_start_on_bus(i2c_periph);
    timeout = I2C_TIMEOUT;
    while(!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
    }
    
    i2c_master_addressing(i2c_periph, i2c_addr, I2C_RECEIVER);
    
    if(number_of_byte == 1) {
        i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
        i2c_stop_on_bus(i2c_periph);
    } else if(number_of_byte == 2) {
        i2c_ackpos_config(i2c_periph, I2C_ACKPOS_NEXT);
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
        i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
    } else {
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);
        i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
    }
    
    while(number_of_byte) {
        if(number_of_byte == 3) {
            timeout = I2C_TIMEOUT;
            while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
                if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
            }
            i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
        }
        if(number_of_byte == 2) {
            timeout = I2C_TIMEOUT;
            while(!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
                if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
            }
            i2c_stop_on_bus(i2c_periph);
        }
        
        timeout = I2C_TIMEOUT;
        while(!i2c_flag_get(i2c_periph, I2C_FLAG_RBNE)) {
            if(--timeout == 0) { i2c_stop_on_bus(i2c_periph); return 0; }
        }
        
        *p_buffer++ = i2c_data_receive(i2c_periph);
        number_of_byte--;
    }
    
    timeout = I2C_TIMEOUT;
    while(I2C_CTL0(i2c_periph) & 0x0200) {
        if(--timeout == 0) break;
    }
    
    i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
    i2c_ackpos_config(i2c_periph, I2C_ACKPOS_CURRENT);
    return 1;
}

void i2c_delay_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer) {
    uint32_t time = 20000;
    i2c_delay(time);
    i2c_byte_write(i2c_periph, i2c_addr, write_address, buffer);
    i2c_delay(time);
}

void i2c_delay_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t* p_buffer, uint8_t number_of_byte) {
    uint32_t time = 20000;
    i2c_delay(time);
    i2c_write(i2c_periph, i2c_addr, write_address, p_buffer, number_of_byte);
    i2c_delay(time);
}
