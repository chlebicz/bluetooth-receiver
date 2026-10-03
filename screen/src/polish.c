#include "screen/polish.h"
#include <stddef.h>

void replace_polish_characters(uint8_t* original)
{
  if (original == NULL)
  {
    return;
  }

  uint8_t* original_ptr = original;
  uint8_t* modified_ptr = original;

  while (*original_ptr != 0)
  {
    uint8_t c = *original_ptr;
    original_ptr++; // original_ptr now points to the next byte

    if (c < 128)
    {
      *modified_ptr = c;
    }
    else if (c == 0xC3)
    {
      if (*original_ptr == 0) break;
      switch (*original_ptr)
      {
        case 0xB3: *modified_ptr = 'o'; original_ptr++; break;
        case 0x93: *modified_ptr = 'O'; original_ptr++; break;
        default:   *modified_ptr = '?';
                   // Only consume if it's a continuation byte
                   if ((*original_ptr & 0xC0) == 0x80) original_ptr++;
                   break;
      }
    }
    else if (c == 0xC4)
    {
      if (*original_ptr == 0) break;
      switch (*original_ptr)
      {
        case 0x85: *modified_ptr = 'a'; original_ptr++; break;
        case 0x84: *modified_ptr = 'A'; original_ptr++; break;
        case 0x87: *modified_ptr = 'c'; original_ptr++; break;
        case 0x86: *modified_ptr = 'C'; original_ptr++; break;
        case 0x99: *modified_ptr = 'e'; original_ptr++; break;
        case 0x98: *modified_ptr = 'E'; original_ptr++; break;
        default:   *modified_ptr = '?'; 
                   if ((*original_ptr & 0xC0) == 0x80) original_ptr++;
                   break; 
      }
    }
    else if (c == 0xC5)
    {
      if (*original_ptr == 0) break;
      switch (*original_ptr)
      {
        case 0x82: *modified_ptr = 'l'; original_ptr++; break;
        case 0x81: *modified_ptr = 'L'; original_ptr++; break;
        case 0x84: *modified_ptr = 'n'; original_ptr++; break;
        case 0x83: *modified_ptr = 'N'; original_ptr++; break;
        case 0x9B: *modified_ptr = 's'; original_ptr++; break;
        case 0x9A: *modified_ptr = 'S'; original_ptr++; break;
        case 0xBA:
        case 0xBC: *modified_ptr = 'z'; original_ptr++; break;
        case 0xB9:
        case 0xBB: *modified_ptr = 'Z'; original_ptr++; break;
        default:   *modified_ptr = '?';
                   if ((*original_ptr & 0xC0) == 0x80) original_ptr++;
                   break;
      }
    }
    else
    {
      *modified_ptr = '?';
      while ((*original_ptr & 0xC0) == 0x80)
      {
        original_ptr++;
      }
    }

    modified_ptr++;
  }

  *modified_ptr = '\0';
}
