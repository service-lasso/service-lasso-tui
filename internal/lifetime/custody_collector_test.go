package lifetime

import("bytes";"encoding/binary";"errors";"os";"testing")
type collectorEndpoint struct{input *bytes.Reader;output bytes.Buffer}
func(e *collectorEndpoint)Read(p []byte)(int,error){return e.input.Read(p)}
func(e *collectorEndpoint)Write(p []byte)(int,error){return e.output.Write(p)}
func collectionInput(body []byte)[]byte{var header [8]byte;binary.LittleEndian.PutUint64(header[:],uint64(len(body)));return append(header[:],body...)}

// These are UNRUN algorithm/source cases. Temporary files/endpoints do not
// authenticate the genuine outside-family owner or installed native route.
func TestLocalCollectionHoldsIndependentFullReadbackBeforeReply(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"local-custody-");if err!=nil{t.Fatal(err)};defer file.Close()
 body:=[]byte("private complete representation");endpoint:=&collectorEndpoint{input:bytes.NewReader(collectionInput(body))}
 collection:=&LocalCollection{Endpoint:endpoint,Storage:file};if err:=PrepareLocalCollection(collection,len(body));err!=nil{t.Fatal(err)}
 if err:=collection.Collect();err!=nil{t.Fatal(err)}
 if !bytes.Equal(collection.Readback,body)||!bytes.Equal(collection.Body,body)||&collection.Body[0]==&collection.Readback[0]||collection.Storage!=file{t.Fatal("actual independent readback custody lost")}
 if endpoint.output.Len()!=8||binary.LittleEndian.Uint64(endpoint.output.Bytes())!=uint64(len(body)){t.Fatal("reply preceded complete body")}
 if err:=collection.Collect();err==nil{t.Fatal("once collection replayed")}
}
func TestLocalCollectionFailedWriteKeepsOriginalAndDoesNotReply(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"local-custody-");if err!=nil{t.Fatal(err)};if err:=file.Close();err!=nil{t.Fatal(err)}
 body:=[]byte("private representation");endpoint:=&collectorEndpoint{input:bytes.NewReader(collectionInput(body))}
 collection:=&LocalCollection{Endpoint:endpoint,Storage:file};if err:=PrepareLocalCollection(collection,len(body));err!=nil{t.Fatal(err)}
 err=collection.Collect();if !errors.Is(err,os.ErrClosed)||collection.Write.Failure==nil||collection.Storage!=file||endpoint.output.Len()!=0||!bytes.Equal(collection.Body,body){t.Fatal("failed physical custody was projected or acknowledged")}
}
func TestLocalCollectionRequiresActualCompletedPreparation(t *testing.T){
 collection:=&LocalCollection{}
 if err:=collection.Collect();err==nil||collection.attempted{t.Fatal("unprepared source entered IO")}
 if err:=PrepareLocalCollection(collection,-1);err==nil{t.Fatal("invalid reservation accepted")}
 if err:=PrepareLocalCollection(collection,0);err!=nil{t.Fatal(err)}
 if err:=PrepareLocalCollection(collection,1);err==nil{t.Fatal("actual reserved buffers replaced")}
}
func TestLocalCollectionUsesSamePreparedFileEndpointAndBackings(t *testing.T){
 file,err:=os.CreateTemp(t.TempDir(),"local-custody-");if err!=nil{t.Fatal(err)};defer file.Close()
 body:=[]byte("actual original representation");endpoint:=&collectorEndpoint{input:bytes.NewReader(collectionInput(body))}
 collection:=&LocalCollection{Endpoint:endpoint,Storage:file};if err:=PrepareLocalCollection(collection,len(body));err!=nil{t.Fatal(err)}
 originalBody,originalReadback:=collection.Body,collection.Readback
 collection.Endpoint=nil;collection.Storage=nil;collection.Body=[]byte("replacement");collection.Readback=[]byte("replacement")
 if err:=collection.Collect();err!=nil{t.Fatal(err)}
 if !bytes.Equal(originalBody,body)||!bytes.Equal(originalReadback,body)||endpoint.output.Len()!=8||collection.preparedStorage!=file{t.Fatal("actual prepared source association replaced")}
}
