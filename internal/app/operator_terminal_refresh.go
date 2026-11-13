package app

import (
 "errors"
 tea "github.com/charmbracelet/bubbletea"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Private in-memory failures preserve original attempt/context and cause across
// later success, navigation and deliberate record clear. This is not durable
// native capture or capacity qualification; raw errors never enter View/JSON.
type operatorTerminalRefreshFailure struct{Epoch,Sequence uint64;Binding api.OperatorContext `json:"-"`;Original api.OperatorOperation;Err error `json:"-"`}
type operatorTerminalRefreshed struct{Epoch,Sequence uint64;Binding api.OperatorContext `json:"-"`;Original api.OperatorOperation;Ticket operatorTicket `json:"-"`;Detail api.OperatorDetail;Err error `json:"-"`}

// One automatic attempt follows the original settled operation, not a browsing
// selection. A failed read remains distinct from completion and is retried only
// by the existing explicit read-refresh gesture; polling never retries it.
func(m *operatorModel)refreshTerminalState(retry bool)tea.Cmd{
 if !m.binding.Valid()||m.operation==nil||!m.operation.Operation.CanClear()||m.terminalRefreshInFlight{return nil}
 original:=m.operation.Operation
 if m.terminalRefreshID==original.OperationID&&(!retry||m.terminalRefreshWarning==""){return nil}
 m.terminalRefreshID=original.OperationID;m.terminalRefreshSequence++
 if allowed,code:=operatorReadAdmission(m.discovery,"detail");!allowed{m.terminalRefreshWarning=api.OperatorMessage(code);return nil}
 m.terminalRefreshInFlight=true
 epoch,sequence,binding,ticket,client,ctx:=m.epoch,m.terminalRefreshSequence,m.binding,m.ticket(),m.client,m.ctx
 return func()tea.Msg{
  result:=operatorTerminalRefreshed{Epoch:epoch,Sequence:sequence,Binding:binding,Original:original,Ticket:ticket}
  before,err:=client.OperatorContext(ctx)
  if err!=nil||before!=binding{result.Err=errors.Join(&api.OperatorError{Stage:"context"},err);return result}
  result.Detail,result.Err=client.OperatorDetail(ctx,original.ServiceID)
  if result.Err!=nil{return result}
  after,err:=client.OperatorContext(ctx)
  if err!=nil||after!=binding{result.Err=errors.Join(&api.OperatorError{Stage:"context"},err)}
  return result
 }
}

// Context bracketing is read consistency, not a server-held authorization lease.
// The current server independently authorizes the real Detail request.
