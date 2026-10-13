package app

import "time"

func operatorSnapshotStatus(snapshot,expiry string,complete,stale,truncated bool,rows int)string{
 if snapshot==""{return ""};status:="incomplete";if complete{status="complete";if rows==0{status="complete; no rows"}};if truncated{status="truncated; omitted rows remain unknown"};if stale||operatorSnapshotExpired(expiry,time.Now()){status="stale; retained rows need refresh"};return "Snapshot: "+status+". Expires: "+expiry
}
func(m operatorModel)snapshotStatus()string{
 if !m.pageSelectionCurrent(){return ""}
 switch m.journey{
 case "overview":p:=m.services;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "graph":p:=m.graph;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,false,len(p.Nodes))
 case "variables":p:=m.variables;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "endpoints":p:=m.endpoints;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "health_history":p:=m.health;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "history":p:=m.history;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "inbox":p:=m.inbox;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "queue":p:=m.queue;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "audit":p:=m.audit;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 case "logs":p:=m.logs;return operatorSnapshotStatus(p.Snapshot,p.Expiry,p.Complete,p.Stale,p.Truncated,len(p.Rows))
 };return ""
}
