//go:build linux

package app

import "os"

func recordCloseSourceFixture()(func(*os.File)error,func()recordStorageOutcome){
 store:=&linuxOperationStore{lock:-1,primaryParentFD:-1,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 return func(file *os.File)error{return store.closeFile("source-fixture-file",file)},store.storageOutcome
}

func recordFinalizerSourceFixture(file *os.File)(func()error,func()recordStorageOutcome){
 store:=&linuxOperationStore{lock:-1,primaryParentFD:-1,target:file,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 return store.close,store.storageOutcome
}

func recordDiscardSourceFixture(file *os.File)(func()error,func()recordStorageOutcome){
 store:=&linuxOperationStore{lock:-1,primaryParentFD:-1,observed:recordStorageOutcome{PublicationEffect:recordUnchanged,CleanupRelease:recordReleased}}
 prepared:=&linuxPrepared{file:file,retained:true}
 return func()error{return store.discardPrepared(prepared)},store.storageOutcome
}