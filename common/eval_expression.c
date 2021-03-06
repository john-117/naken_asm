/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2014 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "eval_expression.h"
#include "get_tokens.h"

struct _operator
{
  int operation;
  int precedence;
};

static int get_operator(char *token, struct _operator *operator)
{
  if (token[1] == 0)
  {
    if (token[0] == '~')
    {
      operator->precedence = PREC_NOT;
      operator->operation = OPER_NOT;
    }
      else
    if (token[0] == '*')
    {
      operator->precedence = PREC_MUL;
      operator->operation = OPER_MUL;
    }
      else
    if (token[0] == '/')
    {
      operator->precedence = PREC_MUL;
      operator->operation = OPER_DIV;
    }
      else
    if (token[0] == '%')
    {
      operator->precedence = PREC_MUL;
      operator->operation = OPER_MOD;
    }
      else
    if (token[0] == '+')
    {
      operator->precedence = PREC_ADD;
      operator->operation = OPER_PLUS;
    }
      else
    if (token[0] == '-')
    {
      operator->precedence = PREC_ADD;
      operator->operation = OPER_MINUS;
    }
      else
    if (token[0] == '&')
    {
      operator->precedence = PREC_AND;  
      operator->operation = OPER_AND;
    }
      else
    if (token[0] == '^')
    {
      operator->precedence = PREC_XOR;
      operator->operation = OPER_XOR;
    }
      else
    if (token[0] == '|')
    {
      operator->precedence = PREC_OR;
      operator->operation = OPER_OR;
    }
      else
    {
      return -1;
    }
  }
    else
  if (token[2] == 0)
  {
    if (token[0] == '<' && token[1] == '<')
    {
      operator->precedence = PREC_SHIFT;
      operator->operation = OPER_LEFT_SHIFT;
    }
      else
    if (token[0] == '>' && token[1] == '>')
    {
      operator->precedence = PREC_SHIFT;
      operator->operation = OPER_RIGHT_SHIFT;
    }
      else
    {
      return -1;
    }
  }
    else
  {
    return -1;
  }

  return 0;
}

static int operate(int a, int b, struct _operator *operator)
{
#ifdef DEBUG
printf(">>> OPERATING ON %d (%d) %d\n", a, operator->operation, b);
#endif

  switch(operator->operation)
  {
    case OPER_NOT:
      return ~a;
    case OPER_MUL:
      return a * b;
    case OPER_DIV:
      return a / b;
    case OPER_MOD:
      return a % b;
    case OPER_PLUS:
      return a + b;
    case OPER_MINUS:
      return a - b;
    case OPER_LEFT_SHIFT:
      return a << b;
    case OPER_RIGHT_SHIFT:
      return a >> b;
    case OPER_AND:
      return a & b;
    case OPER_XOR:
      return a ^ b;
    case OPER_OR:
      return a | b;
    default:
      printf("Internal Error: WTF, bad operator %d\n", operator->operation);
      return 0;
  }
}

static int eval_expression_go(struct _asm_context *asm_context, int *num, struct _operator *last_operator)
{
char token[TOKENLEN];
int token_type;
int num_stack[3];
int num_stack_ptr=1;
struct _operator operator;

#ifdef DEBUG
printf("Enter eval_expression_go,  num=%d\n", *num);
#endif

  memcpy(&operator, last_operator, sizeof(struct _operator));
  num_stack[0] = *num;

  while(1)
  {
#ifdef DEBUG
printf("eval_expression> going to grab a token\n");
#endif
    token_type = get_token(asm_context, token, TOKENLEN);

#ifdef DEBUG
printf("eval_expression> token=%s   num_stack_ptr=%d\n", token, num_stack_ptr);
#endif
    if (token_type == TOKEN_QUOTED)
    {
      if (token[0] == '\\')
      {
        int e = escape_char(asm_context, (unsigned char *)token);
        if (e == 0) return -1;
        if (token[e+1] != 0)
        {
          print_error("Quoted literal too long.", asm_context);
          return -1;
        }
        sprintf(token, "%d", token[e]);
      }
        else
      {
        if (token[1]!=0)
        {
          print_error("Quoted literal too long.", asm_context);
          return -1;
        }
        sprintf(token, "%d", token[0]);
      }

      token_type = TOKEN_NUMBER;
    }

    // Open and close parenthesis
    if (IS_TOKEN(token,'('))
    {
      if (operator.operation == OPER_UNSET && num_stack_ptr == 2)
      {
        // This is probably the x(r12) case.. so this is actually okay
        *num = num_stack[num_stack_ptr-1];
        pushback(asm_context, token, token_type);
        return 0;
      }

      int paren_num = 0;
      struct _operator paren_operator;
      paren_operator.precedence = PREC_UNSET;
      paren_operator.operation = OPER_UNSET;

      if (eval_expression_go(asm_context, &paren_num, &paren_operator)!=0)
      { return -1; }

#ifdef DEBUG
printf("Paren got back %d\n", paren_num);
#endif
      num_stack[num_stack_ptr++] = paren_num;

      token_type = get_token(asm_context, token, TOKENLEN);
      if (!(token[1] == 0 && token[0] == ')'))
      {
        print_error("No matching ')'", asm_context);
        return -1;
      }
      continue;
    }

    if (IS_TOKEN(token,')'))
    {
      pushback(asm_context, token, token_type);
      break;
    }

    // End of expression
    if (IS_TOKEN(token,',') || IS_TOKEN(token,']') || token_type == TOKEN_EOF)
    {
      pushback(asm_context, token, token_type);
      break;
    }
    if (token_type == TOKEN_EOL)
    {
      //asm_context->line++;
      pushback(asm_context, token, token_type);
      break;
    }

    // Read number
    if (token_type == TOKEN_NUMBER)
    {
      if (num_stack_ptr == 3)
      {
        print_error_unexp(token, asm_context);
        return -1;
      }

      num_stack[num_stack_ptr++] = atoi(token);
    }
      else
    if (token_type == TOKEN_SYMBOL)
    {
      if (get_operator(token, &operator)==-1)
      {
        print_error_unexp(token, asm_context);
        return -1;
      }

      if (num_stack_ptr==0)
      {
        printf("Error: Unexpected operator '%s' at %s:%d\n", token, asm_context->filename, asm_context->line);
        return -1;
      }

#ifdef DEBUG
printf("TOKEN %s: precedence %d %d\n", token, last_operator->precedence, operator.precedence);
#endif

      if (last_operator->precedence > operator.precedence)
      {
        if (eval_expression_go(asm_context, &num_stack[num_stack_ptr-1], &operator)==-1)
        {
          return -1;
        }
      }
        else
      if (last_operator->precedence < operator.precedence)
      {
        pushback(asm_context, token, token_type);
        *num = num_stack[num_stack_ptr-1];
        return 0;
      }
        else
      {
        num_stack[num_stack_ptr-2] = operate(num_stack[num_stack_ptr-2], num_stack[num_stack_ptr-1], last_operator);
        num_stack_ptr--;
        memcpy(last_operator, &operator, sizeof(struct _operator));
      }
    }
      else
    {
      if (asm_context->pass != 1)
      {
        print_error_unexp(token, asm_context);
      }
      return -1;
    }
  }

#ifdef DEBUG
printf("going to leave %d\n", last_operator->operation);
int i;
for (i = 0; i < num_stack_ptr; i++) printf("-- %d\n", num_stack[i]);
#endif

  if (last_operator->operation != OPER_UNSET)
  {
    num_stack[num_stack_ptr-2] = operate(num_stack[num_stack_ptr-2], num_stack[num_stack_ptr-1], last_operator);
    num_stack_ptr--;
  }

  *num = num_stack[num_stack_ptr-1];

  return 0;
}

int eval_expression(struct _asm_context *asm_context, int *num)
{
struct _operator operator;

  *num = 0;
  operator.precedence = PREC_UNSET;
  operator.operation = OPER_UNSET;
  return eval_expression_go(asm_context, num, &operator);
}


