
#include "interrupts.h"

void printf(char* str);


InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];


uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber,uint32_t esp)
{
    printf("  interrupt!");

    return esp;
}

InterruptManager::InterruptManager(GlobalDescriptorTable* gdt)
: picMasterCommand(0x20), // 初始化端口号
  picMasterData(0x21),
  picSlaveCommand(0xA0),
  picSlaveData(0xA1)
{
    uint16_t CodeSegment = gdt->CodeSegmentSelector();
    const uint8_t IDT_INTERRUPT_GATE = 0xE;
    for(uint16_t i = 0; i< 256; i++)

	
        SetInterruptDescriptorTableEntry(
	i, CodeSegment, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);

    SetInterruptDescriptorTableEntry(
    0x20, CodeSegment,&HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(
    0x21, CodeSegment,&HandleInterruptRequest0x01,0,IDT_INTERRUPT_GATE);

    picMasterCommand.Write(0x11);
    picSlaveCommand.Write(0x11);
    picMasterData.Write(0x20); //master pic和slave pic都可以使用八个中断
    picSlaveData.Write(0x28);

    picMasterData.Write(0x04);
    picSlaveData.Write(0x02);

    picMasterData.Write(0x01);
    picSlaveData.Write(0x01);

    picMasterData.Write(0x00);
    picSlaveData.Write(0x00);


    InterruptDescriptorTablePointer idt;
    idt.size  = 256*sizeof(GateDescriptor) - 1;
    idt.base  = (uint32_t)interruptDescriptorTable;

    asm volatile("lidt %0" : : "m" (idt));//通知编译器加载idt表
}

InterruptManager::~InterruptManager()
{
}

void InterruptManager::Activate(){

    asm("sti");

}


// BEGIN
void InterruptManager::SetInterruptDescriptorTableEntry(
	uint8_t interrupt,
	uint16_t CodeSegment, 
	void (*handler)(), 
	uint8_t DescriptorPrivilegeLevel, 
	uint8_t DescriptorType)
{
    // address of pointer to code segment (relative to global descriptor table)
    // and address of the handler (relative to segment)
    interruptDescriptorTable[interrupt].handlerAddressLowBits = ((uint32_t) handler) & 0xFFFF;
    interruptDescriptorTable[interrupt].handlerAddressHighBits = (((uint32_t) handler) >> 16) & 0xFFFF;
    interruptDescriptorTable[interrupt].gdt_codeSegmentSelector = CodeSegment;

    const uint8_t IDT_DESC_PRESENT = 0x80;
    interruptDescriptorTable[interrupt].access = 
      IDT_DESC_PRESENT | ((DescriptorPrivilegeLevel & 3) << 5) | DescriptorType;

    interruptDescriptorTable[interrupt].reserved = 0;
}
// END



















