package localereader

import("bytes";"testing")

func TestUTF8ASCII(t *testing.T){input:=[]byte("ASCII input\r\n");actual,err:=UTF8(input);if err!=nil||!bytes.Equal(actual,input){t.Fatal("ASCII public projection changed",err)}}