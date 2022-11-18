
#include "interrupts.h"
void printf(char* str);

InterruptHandler::InterruptHandler(uint8_t interruptNumber,
InterruptManager* interruptManager)
{
    this->interruptNumber = interruptNumber;
    this->interruptManager = interruptManager;
    interruptManager->handlers[interruptNumber] = this;
}
	
InterruptHandler::~InterruptHandler(){
    if(interruptManager->handlers[interruptNumber] == this)
	interruptManager->handlers[interruptNumber] = 0;
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp)
{
    return esp;
}


InterruptManager::GateDescriptor InterruptManager::interruptDescriptorTable[256];


InterruptManager* InterruptManager::ActiveInterruptManager=0;



InterruptManager::InterruptManager(GlobalDescriptorTable* gdt)
: picMasterCommand(0x20), // 初始化端口号
  picMasterData(0x21),
  picSlaveCommand(0xA0),
  picSlaveData(0xA1)
{
    uint16_t CodeSegment = gdt->CodeSegmentSelector();
    const uint8_t IDT_INTERRUPT_GATE = 0xE;
    for(uint16_t i = 0; i< 256; i++)
    {
	handlers[i] = 0;
        SetInterruptDescriptorTableEntry(
	i, CodeSegment, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
    }
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

    printf("this is Activate\n");
    if(ActiveInterruptManager!=0){//说明此时系统中存在中断
	ActiveInterruptManager->Deactivate();
    }
    ActiveInterruptManager = this; //静态中断处理器指向的永远是当前处理的中断
    asm("sti");

}

void InterruptManager::Deactivate(){ //收回指向当前实例的指针
    printf("this is Deactivate\n");
    if(ActiveInterruptManager == this){
	ActiveInterruptManager=0;
	asm("cli");
    }

}


uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber,uint32_t esp)
{
    char* hex = "this is handleInterrupt   \n";
    hex[23] = '0'+interruptNumber/100;
    hex[24] = '0'+(interruptNumber/10)%10;
    hex[25] = '0'+interruptNumber%10;
    //printf(hex);

    if(ActiveInterruptManager!=0) //中断处理器不为0,即存在中断
    {
	return ActiveInterruptManager->DoHandleInterrupt(interruptNumber,esp);
    }
    return esp;
}


uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber,uint32_t esp)
{

    /**/if(handlers[interruptNumber] !=0 ) 
    {
	esp = handlers[interruptNumber]->HandleInterrupt(esp);
    }
    else if(interruptNumber != 0x20 ){
	char* foo = "interrupt 0x00";
	char *hex = "0123456789ABCDEF";
	foo[12] = hex[(interruptNumber >> 4) & 0x0F];
	foo[13] = hex[interruptNumber & 0x0F];
	printf(foo);

    }
    
    if(0x20 <= interruptNumber && interruptNumber<0x30){
	picMasterCommand.Write(0x20);
	if(0x28 <= interruptNumber){
	    picSlaveCommand.Write(0x20);
	}
    }

    return esp;
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



















