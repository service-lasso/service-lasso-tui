package app

import (
	"encoding/json"
 "time"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type operatorPageState[T any]struct{
	Snapshot,Revision,Expiry string
	Rows []T
	Next *string
	Complete,Truncated,Stale bool
	seenRows map[string]bool
	seenCursors map[string]bool
}
func operatorSnapshotExpired(expiry string,now time.Time)bool{if expiry==""{return false};return api.OperatorSnapshotExpiredAt(expiry,now)}
func(p *operatorPageState[T])expire(now time.Time){if operatorSnapshotExpired(p.Expiry,now){p.Stale=true}}
func(p *operatorPageState[T])append(page api.OperatorPage[T],cursor *string,rowID func(T)string)error{
 candidate:=*p;candidate.Rows=append([]T(nil),p.Rows...);candidate.seenRows=operatorSeenCopy(p.seenRows);candidate.seenCursors=operatorSeenCopy(p.seenCursors)
 if err:=candidate.appendValidated(page,cursor,rowID);err!=nil{p.Stale=true;p.Complete=false;return err};*p=candidate;return nil
}
func operatorSeenCopy(source map[string]bool)map[string]bool{copy:=make(map[string]bool,len(source));for key,value:=range source{copy[key]=value};return copy}
func(p *operatorPageState[T])appendValidated(page api.OperatorPage[T],cursor *string,rowID func(T)string)error{
 if operatorSnapshotExpired(page.ExpiresAt,time.Now()){p.Stale=true;return &api.OperatorPublicError{Code:"snapshot_expired"}}
	if page.NextCursor!=nil&&len(page.Rows)==0{p.Stale=true;return &api.OperatorError{Stage:"response"}}
	if cursor==nil{*p=operatorPageState[T]{Snapshot:page.SnapshotID,Revision:page.SourceRevision,Expiry:page.ExpiresAt,seenRows:map[string]bool{},seenCursors:map[string]bool{}}}else if p.Stale||p.Next==nil||*p.Next!=*cursor||page.SnapshotID!=p.Snapshot||page.SourceRevision!=p.Revision||page.ExpiresAt!=p.Expiry{p.Stale=true;return &api.OperatorError{Stage:"response"}}
	if page.NextCursor!=nil&&(p.seenCursors[*page.NextCursor]||cursor!=nil&&*page.NextCursor==*cursor){p.Stale=true;return &api.OperatorError{Stage:"response"}}
	if len(p.Rows)+len(page.Rows)>4096{p.Stale=true;return &api.OperatorPublicError{Code:"limit_exceeded"}}
	for _,row:=range page.Rows{id:=rowID(row);if id==""||p.seenRows[id]{p.Stale=true;return &api.OperatorError{Stage:"response"}};p.seenRows[id]=true}
	p.Rows=append(p.Rows,page.Rows...);p.Next=page.NextCursor;p.Truncated=p.Truncated||page.Truncated;p.Complete=page.NextCursor==nil
	if cursor!=nil{p.seenCursors[*cursor]=true};return nil
}

type operatorGraphState struct{
	Snapshot,Revision,Expiry string
	Next *string
	Nodes []api.OperatorNode
	Edges []api.OperatorEdge
	Complete,Stale bool
	lastNode *api.OperatorNode
	lastTotal,lastOffset int
	lastComplete bool
	lastEdge *api.OperatorEdge
	seenCursors map[string]bool
}
func(g *operatorGraphState)append(page api.OperatorPage[api.OperatorGraphGroup],cursor *string)error{
 candidate:=*g;candidate.Nodes=append([]api.OperatorNode(nil),g.Nodes...);candidate.Edges=append([]api.OperatorEdge(nil),g.Edges...);candidate.seenCursors=operatorSeenCopy(g.seenCursors)
 if err:=candidate.appendValidated(page,cursor);err!=nil{g.Stale=true;g.Complete=false;return err};*g=candidate;return nil
}
func(g *operatorGraphState)appendValidated(page api.OperatorPage[api.OperatorGraphGroup],cursor *string)error{
	bad:=func()error{g.Stale=true;g.Complete=false;return &api.OperatorError{Stage:"response"}}
 if page.NextCursor!=nil&&len(page.Rows)==0{return bad()}
 if operatorSnapshotExpired(page.ExpiresAt,time.Now()){g.Stale=true;return &api.OperatorPublicError{Code:"snapshot_expired"}}
	if page.Truncated{return bad()}
	if cursor==nil{*g=operatorGraphState{Snapshot:page.SnapshotID,Revision:page.SourceRevision,Expiry:page.ExpiresAt,seenCursors:map[string]bool{}}}else if g.Stale||g.Next==nil||*g.Next!=*cursor||page.SnapshotID!=g.Snapshot||page.SourceRevision!=g.Revision||page.ExpiresAt!=g.Expiry{return bad()}
	if page.NextCursor!=nil&&(g.seenCursors[*page.NextCursor]||cursor!=nil&&*page.NextCursor==*cursor){return bad()}
 progress:=len(g.Nodes)+len(g.Edges)
	for _,fragment:=range page.Rows{
		if g.lastNode==nil||fragment.Node.ServiceID!=g.lastNode.ServiceID{
			if g.lastNode!=nil&&(!g.lastComplete||fragment.Node.ServiceID<=g.lastNode.ServiceID){return bad()}
			if fragment.EdgeOffset!=0{return bad()};node:=fragment.Node;g.lastNode=&node;g.lastTotal=fragment.EdgeTotal;g.lastOffset=0;g.lastComplete=false;g.lastEdge=nil;g.Nodes=append(g.Nodes,node)
		}else{
			left,_:=json.Marshal(g.lastNode);right,_:=json.Marshal(fragment.Node)
			if g.lastComplete||string(left)!=string(right)||fragment.EdgeTotal!=g.lastTotal||fragment.EdgeOffset!=g.lastOffset{return bad()}
		}
		for _,edge:=range fragment.Edges{if g.lastEdge!=nil&&(edge.FromServiceID<g.lastEdge.FromServiceID||edge.FromServiceID==g.lastEdge.FromServiceID&&edge.Kind<=g.lastEdge.Kind){return bad()};copy:=edge;g.lastEdge=&copy;g.Edges=append(g.Edges,edge)}
		g.lastOffset=fragment.EdgeOffset+len(fragment.Edges);g.lastComplete=fragment.Complete
		if len(g.Nodes)>4096||len(g.Edges)>4096{return bad()}
	}
 if page.NextCursor!=nil&&len(g.Nodes)+len(g.Edges)==progress{return bad()}
	if page.NextCursor==nil&&g.lastNode!=nil&&!g.lastComplete{return bad()}
	g.Next=page.NextCursor;g.Complete=page.NextCursor==nil;if cursor!=nil{g.seenCursors[*cursor]=true};return nil
}
