#include "gdt.h"

/*
不清楚
asm volatile("lgdt (%0)": :"p" (((uint8_t *) i)+2)); 
这行代码，但修改i后的2即会出错
且修改i的元素顺序也会出“DBGF: No debugger attached, waiting 1 second for one to attach (event=11)”bug
猜测时编译器加载gdt表出现的问题
*/
GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegmentSelector(0, 0, 0),
        unusedSegmentSelector(0, 0, 0),
        codeSegmentSelector(0, 64*1024*1024, 0x9A),
        dataSegmentSelector(0, 64*1024*1024, 0x92)
{
    uint32_t i[2];
    i[1] = (uint32_t)this; // 表的指针
    i[0] = sizeof(GlobalDescriptorTable) << 16; // 表的大小
	// 11.14解决了持续两周的问题，即i[0]和i[1]写反了
    // 汇编代码：告诉编译器加载这个GlobalDescriptorTable
    asm volatile("lgdt (%0)": :"p" (((uint8_t *) i)+2)); 
}

GlobalDescriptorTable::~GlobalDescriptorTable()
{
}

uint16_t GlobalDescriptorTable::DataSegmentSelector()
{
// 返回数据的offset
    return (uint8_t*)&dataSegmentSelector - (uint8_t*)this;
}

uint16_t GlobalDescriptorTable::CodeSegmentSelector()
{
    return (uint8_t*)&codeSegmentSelector - (uint8_t*)this;
}


GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type)
{
//生成gdt表的存储数据结构
    uint8_t* target = (uint8_t*)this;

    if (limit <= 65536) // 代表可以用16bit的数据表示
    {
        // 16-bit address space
        target[6] = 0x40;
    }
    else
    {
        // 32-bit address space
        // Now we have to squeeze the (32-bit) limit into 2.5 regiters (20-bit).
        // This is done by discarding the 12 least significant bits, but this
        // is only legal, if they are all ==1, so they are implicitly still there

        // so if the last bits aren't all 1, we have to set them to 1, but this
        // would increase the limit (cannot do that, because we might go beyond
        // the physical limit or get overlap with other segments) so we have to
        // compensate this by decreasing a higher bit (and might have up to
        // 4095 wasted bytes behind the used memory)

        if((limit & 0xFFF) != 0xFFF)
            limit = (limit >> 12)-1;
        else
            limit = limit >> 12;

        target[6] = 0xC0;
    }

    // Encode the limit
    target[0] = limit & 0xFF; // 取低八位
    target[1] = (limit >> 8) & 0xFF; // 取二进制中第二个八位（9～16）
    target[6] |= (limit >> 16) & 0xF;

    // Encode the base
    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF;
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;

    // Type
    target[5] = type;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Base()
{
    uint8_t* target = (uint8_t*)this;

    uint32_t result = target[7];
    result = (result << 8) + target[4];
    result = (result << 8) + target[3];
    result = (result << 8) + target[2];

    return result;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit()
{
    uint8_t* target = (uint8_t*)this;

    uint32_t result = target[6] & 0xF;
    result = (result << 8) + target[1];
    result = (result << 8) + target[0];

    if((target[6] & 0xC0) == 0xC0)
        result = (result << 12) | 0xFFF;

    return result;
}




