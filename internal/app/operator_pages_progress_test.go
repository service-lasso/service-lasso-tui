package app

import (
 "testing"
 "github.com/service-lasso/service-lasso-tui/internal/api"
)

// Accumulator source cases only; these do not confer source disclosure,
// snapshot issuance or authenticated installed Core/native authority.
func TestOperatorGraphContinuationRequiresActualProgress(t *testing.T){
 first,next:="first","next"
 node:=api.OperatorNode{ServiceID:"service",Role:"service",Presence:"present",Availability:api.OperatorAvailability{Supported:true,Available:true}}
 firstEdge:=api.OperatorEdge{FromServiceID:"dependency-a",ToServiceID:"service",Kind:"service",State:"satisfied"}
 fragment:=api.OperatorGraphGroup{Node:node,Edges:[]api.OperatorEdge{firstEdge},EdgeTotal:2}
 page:=api.OperatorPage[api.OperatorGraphGroup]{SnapshotID:"snapshot",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorGraphGroup{fragment},NextCursor:&first}
 state:=operatorGraphState{}
 if err:=state.append(page,nil);err!=nil{t.Fatal(err)}
 // The API decoder already rejects this malformed empty fragment. The
 // accumulator still owns its own finite progress and prior-row custody.
 page.NextCursor=&next;page.Rows[0].Edges=[]api.OperatorEdge{};page.Rows[0].EdgeOffset=1
 if err:=state.append(page,&first);err==nil{t.Fatal("repeated zero-edge fragment grew continuation state")}
 if !state.Stale||len(state.Nodes)!=1||len(state.Edges)!=1||state.Edges[0]!=firstEdge||state.Next==nil||*state.Next!=first||len(state.seenCursors)!=0{t.Fatal("denied fragment changed prior snapshot/cursor custody")}
 page.NextCursor=&first;page.Rows[0].Edges=[]api.OperatorEdge{firstEdge};page.Rows[0].EdgeOffset=0
 if err:=state.append(page,nil);err!=nil{t.Fatal("deliberate fresh snapshot denied",err)}
 page.NextCursor=nil;page.Rows[0].Edges=[]api.OperatorEdge{{FromServiceID:"dependency-b",ToServiceID:"service",Kind:"service",State:"satisfied"}};page.Rows[0].EdgeOffset=1;page.Rows[0].Complete=true
 if err:=state.append(page,&first);err!=nil||!state.Complete||len(state.Edges)!=2{t.Fatal("useful progressing continuation denied",err)}
 page.Rows[0].Edges=[]api.OperatorEdge{};page.Rows[0].EdgeOffset=0;page.Rows[0].EdgeTotal=0
 if err:=state.append(page,nil);err!=nil||!state.Complete||len(state.Nodes)!=1{t.Fatal("isolated authorized node denied",err)}
}

func TestOperatorRejectedInitialPageRetainsPriorRows(t *testing.T){
 cursor:="cursor";state:=operatorPageState[string]{}
 page:=api.OperatorPage[string]{SnapshotID:"original",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]string{"old"},NextCursor:&cursor}
 identity:=func(value string)string{return value}
 if err:=state.append(page,nil,identity);err!=nil{t.Fatal(err)}
 page.SnapshotID="replacement";page.Rows=[]string{"new","new"};page.NextCursor=nil
 if err:=state.append(page,nil,identity);err==nil{t.Fatal("duplicate page admitted")}
 if !state.Stale||state.Snapshot!="original"||len(state.Rows)!=1||state.Rows[0]!="old"||len(state.seenRows)!=1||!state.seenRows["old"]{t.Fatal("failed refresh discarded prior rows or retained rejected row authority")}
}

func TestOperatorRejectedGraphSuffixRetainsPriorRows(t *testing.T){
 cursor:="cursor";state:=operatorGraphState{}
 group:=func(id string)api.OperatorGraphGroup{return api.OperatorGraphGroup{Node:api.OperatorNode{ServiceID:id,Role:"service",Presence:"present",Availability:api.OperatorAvailability{Supported:true,Available:true}},Edges:[]api.OperatorEdge{},Complete:true}}
 page:=api.OperatorPage[api.OperatorGraphGroup]{SnapshotID:"original",SourceRevision:"revision",ExpiresAt:"2099-10-08T00:00:00.000Z",Rows:[]api.OperatorGraphGroup{group("a")},NextCursor:&cursor}
 if err:=state.append(page,nil);err!=nil{t.Fatal(err)}
 page.Rows=[]api.OperatorGraphGroup{group("c"),group("b")};page.NextCursor=nil
 if err:=state.append(page,&cursor);err==nil{t.Fatal("out-of-order suffix admitted")}
 if !state.Stale||len(state.Nodes)!=1||state.Nodes[0].ServiceID!="a"||state.lastNode==nil||state.lastNode.ServiceID!="a"||state.Next==nil||*state.Next!=cursor||len(state.seenCursors)!=0{t.Fatal("failed suffix leaked partial rows or cursor authority")}
}
