#pragma once
#ifndef DEFINE_H
#define DEFINE_H

// CPU variable definition

// register
#define AL		0
#define AX		1
#define AH		2
#define EAX		3

#define CL		4
#define CX		5
#define CH		6
#define ECX		7

#define DL		8
#define DX		9
#define DH		10
#define EDX		11

#define BL		12
#define BX		13
#define BH		14
#define EBX		15

#define SP		16
#define ESP		17
#define BP		18
#define EBP		19
#define SI		20
#define ESI		21
#define DI		22
#define EDI		23

#define ES		24
#define CS		25
#define SS		26
#define DS		27
#define FS		28
#define GS		29

// EFLAGS
#define OF		30
#define SF		31
#define ZF		32
#define AF		33
#define PF		34
#define CF		35
#define TF		36
#define IF		37
#define DF		38
#define NT		39
#define RF		40

// CR
#define CR0		41
#define CR1		42
#define CR2		43
#define CR3		44
#define CR4		45
#define CR5		46
#define CR6		47
#define CR7		48

// DR
#define DR0		49
#define DR1		50
#define DR2		51
#define DR3		52
#define DR4		53
#define DR5		54
#define DR6		55
#define DR7		56
#define V_MEM	57

#define VAR_LENGTH 58

extern char *var_names[VAR_LENGTH];

#endif