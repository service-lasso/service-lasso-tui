package api

import (
 "errors"
 "testing"
 "time"
)

func TestOperatorSnapshotMillisecondExpiryBoundary(t *testing.T){
 deadline:=time.Date(2026,10,8,12,30,0,120000000,time.UTC);spelling:="2026-10-08T12:30:00.120Z"
 if OperatorSnapshotExpiredAt(spelling,deadline.Add(-time.Nanosecond)){t.Fatal("valid final instant before expiry denied")}
 if !OperatorSnapshotExpiredAt(spelling,deadline)||!OperatorSnapshotExpiredAt(spelling,deadline.Add(time.Nanosecond)){t.Fatal("equal or passed snapshot deadline admitted")}
 if !OperatorSnapshotExpiredAt("2026-10-08T12:30:00.12Z",deadline.Add(-time.Second)){t.Fatal("noncanonical timestamp became lifetime authority")}
}

func TestOperatorOriginalPreviewExpiryDoesNotRemint(t *testing.T){
 deadline:=time.Date(2026,10,8,12,30,0,120000000,time.UTC)
 confirmationExpiry:="2026-10-08T12:30:00.120Z";confirmationID:="confirmation";phrase:="confirm service-start 0123456789abcdef0123456789abcdef"
 preview:=OperatorPreview{PlanID:"plan",ServiceID:"service",ActionID:"service_start",DefinitionRevision:"definition",ExpectedRevision:"revision",Generation:"generation",ModelEpoch:"epoch",Targets:[]string{"service"},Effects:[]string{},Availability:OperatorAvailability{Supported:true,Available:true},ExpiresAt:"2026-10-08T12:31:00.120Z",Confirmation:OperatorConfirmation{Required:true,ID:&confirmationID,PhraseKey:&phrase,ExpiresAt:&confirmationExpiry}}
 if !operatorPreviewValid(preview){t.Fatal("expiry fixture is not an admitted preview")}
 if err:=operatorPreviewExpiryAt(preview,deadline.Add(-time.Nanosecond));err!=nil{t.Fatal(err)}
 var public *OperatorPublicError
 if err:=operatorPreviewExpiryAt(preview,deadline);!errors.As(err,&public)||public.Code!="confirmation_expired"{t.Fatal("confirmation deadline did not require a new preview")}
 if preview.PlanID!="plan"||preview.ExpiresAt!="2026-10-08T12:31:00.120Z"||*preview.Confirmation.ExpiresAt!=confirmationExpiry{t.Fatal("original preview was reminted")}
 preview.Confirmation=OperatorConfirmation{}
 if !operatorPreviewValid(preview){t.Fatal("unconfirmed preview fixture is invalid")}
 if err:=operatorPreviewExpiryAt(preview,deadline.Add(time.Minute));!errors.As(err,&public)||public.Code!="binding_stale"{t.Fatal("expired original binding admitted")}
}
