package api

import (
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"strings"
	"testing"
)

// Source cases remain UNRUN; these are not native capture qualification.
func TestReconciliationReadRetainsReadAndCloseOriginals(t *testing.T) {
	readCause := errors.New("private-reconciliation-read")
	closeCause := errors.New("private-reconciliation-close")
	body := &retainedCloseBody{reader:retainedErrorReader{cause:readCause},cause:closeCause}
	client, err := NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return &http.Response{StatusCode:200,Header:http.Header{},Body:body},nil})},"")
	if err != nil { t.Fatal(err) }
	binding, admitted, failure := client.ReconciliationContext()
	if binding!="" || admitted || body.closed!=1 || !errors.Is(failure,readCause) || !errors.Is(failure,closeCause) { t.Fatal("read/close failure lost original or admitted binding") }
	encoded, err := json.Marshal(failure)
	if err!=nil { t.Fatal(err) }
	for _, projection := range []string{failure.Error(),SafeError(failure),string(encoded)} {
		if strings.Contains(projection,"private-reconciliation") { t.Fatal("private reconciliation cause leaked") }
	}
}

func TestReconciliationTransportOriginalIsRetained(t *testing.T) {
	cause := errors.New("private-reconciliation-transport")
	client, err := NewClient("http://127.0.0.1:1",&http.Client{Transport:retainedErrorTransport(func(*http.Request)(*http.Response,error){return nil,cause})},"")
	if err!=nil { t.Fatal(err) }
	binding, admitted, failure := client.ReconciliationContext()
	if binding!="" || admitted || !errors.Is(failure,cause) || strings.Contains(SafeError(failure),cause.Error()) { t.Fatal("transport failure was erased or exposed") }
}

func TestClosedContextRetainsParserFailuresWithoutInventingSemanticCauses(t *testing.T) {
	admitted, failure := closedContextJSON([]byte(`{"contractVersion":@}`))
	var syntax *json.SyntaxError
	if admitted || !errors.As(failure,&syntax) { t.Fatal("actual token SyntaxError was lost") }
	admitted, failure = closedContextJSON([]byte(`{`))
	if admitted || !errors.Is(failure,io.EOF) { t.Fatal("actual premature EOF was lost") }
	admitted, failure = closedContextJSON([]byte(`{"unknown":true}`))
	if admitted || failure!=nil { t.Fatal("semantic refusal manufactured a parser cause") }
}
