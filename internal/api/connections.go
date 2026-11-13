package api

import (
 "github.com/service-lasso/service-lasso-tui/internal/lifetime"
	"io"
 "errors"
	"os"
	"sort"
	"strings"
)

// ConnectionProfile is deliberately metadata-only. tokenEnv names the process
// environment variable which holds the Core local-admin token; the token is
// never written to the connection file or accepted as a command-line value.
type ConnectionProfile struct {
	URL      string   `json:"url"`
	TokenEnv string   `json:"tokenEnv"`
	AuthMode AuthMode `json:"authMode,omitempty"`
	Scopes   []string `json:"scopes,omitempty"`
}

type connectionFile struct {
	Default  string                       `json:"defaultProfile"`
	Profiles map[string]ConnectionProfile `json:"profiles"`
}

// ConnectionOptions applies deterministic launch precedence:
// flags > SERVICE_LASSO_API_* environment > selected profile > loopback default.
// Token values themselves only ever come from a named process environment variable.
type ConnectionOptions struct {
 Receiver *lifetime.Receiver
	ConfigPath string
	Profile    string
	APIURL     string
	TokenEnv   string
	Env        func(string) string
}

type ConnectionManager struct {
 receiver *lifetime.Receiver
	profiles map[string]ConnectionProfile
	current  string
	env      func(string) string
 operatorV2 bool
}

func ResolveConnections(options ConnectionOptions) (*ConnectionManager, *Client, error) {
 return resolveConnections(options,false)
}

func ResolveOperatorConnections(options ConnectionOptions)(*ConnectionManager,*Client,error){
 return resolveConnections(options,true)
}

func resolveConnections(options ConnectionOptions,operatorV2 bool)(*ConnectionManager,*Client,error){
 return resolveConnectionsWithOpener(options,operatorV2,func(path string)(io.ReadCloser,error){return os.Open(path)})
}

// This caller-local seam preserves the actual file/read/close observations.
// It supplies no complete-file, durable capture or later manager lifetime claim.
type connectionInputObservation struct{
 Path string
 File io.ReadCloser
 Raw []byte
 Limit int
 OpenAttempted,ReadAttempted,CloseAttempted bool
 OpenError,ReadError,CloseError error
}
func retainConnectionInput(err error,observation *connectionInputObservation)error{
 if err==nil||observation==nil{return err}
 switch failure:=err.(type){case *OperatorError:failure.input=observation;case *ConfigurationError:failure.input=observation}
 return err
}
func resolveConnectionsWithOpener(options ConnectionOptions,operatorV2 bool,open func(string)(io.ReadCloser,error))(managerResult *ConnectionManager,clientResult *Client,failure error){
	occurrence:=&connectionOccurrence{Options:options,Opener:open};if err:=options.Receiver.Enroll(&occurrence.Original,"api.connections",occurrence);err!=nil{return nil,nil,err};defer func(){occurrence.Manager=managerResult;occurrence.Client=clientResult;occurrence.Failure=failure}()
 var observation *connectionInputObservation
	defer func(){failure=retainConnectionInput(failure,observation)}()
	env := options.Env
	if env == nil {
		env = os.Getenv
	}
	path := firstNonEmpty(options.ConfigPath, env("SERVICE_LASSO_CONNECTIONS_CONFIG"))
	profiles := map[string]ConnectionProfile{}
	defaultProfile := ""
	if path != "" {
		observation=&connectionInputObservation{Path:path,Limit:maxResponseBytes,OpenAttempted:true};occurrence.Input=observation
		file, err := open(path)
		observation.File=file;observation.OpenError=err
		if err != nil {
			return nil, nil, &OperatorError{Stage: "configuration", cause: err}
		}
		observation.ReadAttempted=true
		contents, readErr := io.ReadAll(io.LimitReader(file, maxResponseBytes+1))
		observation.Raw=contents;observation.ReadError=readErr
		observation.CloseAttempted=true
		closeErr:=file.Close();observation.CloseError=closeErr;err=errors.Join(readErr,closeErr)
		if err != nil { return nil, nil, &OperatorError{Stage: "configuration", cause: err} }
		var config connectionFile;occurrence.Destination=&config
		if err := decodeResponse(contents, &config); err != nil {
			return nil, nil, &OperatorError{Stage: "configuration", cause: err}
		}
		if len(config.Profiles) == 0 || len(config.Profiles) > 32 {
			return nil, nil, &OperatorError{Stage: "configuration"}
		}
		if err := validateLoadedProfilesForMode(config.Profiles,operatorV2); err != nil {
			return nil, nil, err
		}
		profiles, defaultProfile = config.Profiles, config.Default
	}
	selected := firstNonEmpty(options.Profile, env("SERVICE_LASSO_API_PROFILE"), defaultProfile)
	profile := ConnectionProfile{}
	if selected != "" {
		var ok bool
		profile, ok = profiles[selected]
		if !ok {
			return nil, nil, &OperatorError{Stage: "configuration"}
		}
	} else {
		selected = "default"
		profile = profiles[selected]
	}
	apiURL := firstNonEmpty(options.APIURL, env("SERVICE_LASSO_API_URL"), profile.URL, "http://127.0.0.1:17883")
	tokenEnv := firstNonEmpty(options.TokenEnv, env("SERVICE_LASSO_API_TOKEN_ENV"), profile.TokenEnv, "SERVICE_LASSO_API_TOKEN")
	profiles[selected] = ConnectionProfile{URL: apiURL, TokenEnv: tokenEnv, AuthMode: profile.AuthMode, Scopes: profile.Scopes}
	manager := &ConnectionManager{profiles: profiles, current: selected, env: env,operatorV2:operatorV2,receiver:options.Receiver}
	client, err := manager.Client(selected)
	if err != nil {
		return nil, nil, err
	}
	return manager, client, nil
}

func firstNonEmpty(values ...string) string {
	for _, value := range values {
		if strings.TrimSpace(value) != "" {
			return strings.TrimSpace(value)
		}
	}
	return ""
}

func (m *ConnectionManager) Names() []string {
	names := make([]string, 0, len(m.profiles))
	for name := range m.profiles {
		names = append(names, name)
	}
	sort.Strings(names)
	return names
}

func (m *ConnectionManager) Current() string { return m.current }

func (m *ConnectionManager) Client(name string) (*Client, error) {
	profile, ok := m.profiles[name]
	if !ok {
		return nil, &OperatorError{Stage: "configuration"}
	}
	mode, err := validateProfileAdmissionForMode(profile,m.operatorV2)
	if err != nil {
		return nil, err
	}
	if profile.TokenEnv == "" {
		return nil, &OperatorError{Stage: "configuration"}
	}
	token := m.env(profile.TokenEnv)
	if token == "" {
		return nil, &OperatorError{Stage: "configuration"}
	}
	if m.operatorV2{client,err:=NewOperatorClientWithAuth(profile.URL,nil,token,mode,profile.Scopes...);if client!=nil{client.receiver=m.receiver};return client,err}
	client,err:=NewClientWithAuth(profile.URL,nil,token,mode,profile.Scopes...);if client!=nil{client.receiver=m.receiver};return client,err
}

func validateLoadedProfiles(profiles map[string]ConnectionProfile) error {
 return validateLoadedProfilesForMode(profiles,false)
}

func validateLoadedProfilesForMode(profiles map[string]ConnectionProfile,operatorV2 bool)error{
	for name, profile := range profiles {
		if !safeIdentifier(name, 80) || len(profile.TokenEnv) > 128 || len(profile.Scopes) > 16 { return &OperatorError{Stage: "configuration"} }
		// An incomplete profile may receive a loopback URL through normal
		// resolution. Once it names a URL, admit it before any profile can be
		// selected and before its credential reference is read.
		if profile.URL == "" {
			continue
		}
		if _, err := validateProfileAdmissionForMode(profile,operatorV2); err != nil {
			return err
		}
	}
	return nil
}

func validateProfileAdmission(profile ConnectionProfile) (AuthMode, error) {
 return validateProfileAdmissionForMode(profile,false)
}

func validateProfileAdmissionForMode(profile ConnectionProfile,operatorV2 bool)(AuthMode,error){
	mode := profile.AuthMode
	if mode == "" {
		mode = AuthModeLocalAdmin
	}
	_,err:=newClientWithAuth(profile.URL,nil,"",mode,operatorV2,profile.Scopes...)
	if err != nil {
		return "", err
	}
	return mode, nil
}

func hasRequiredScopes(scopes []string) bool {
	needed := map[string]bool{"service-lasso:read": false, "service-lasso:lifecycle:write": false}
	for _, scope := range scopes {
		if _, ok := needed[scope]; ok {
			needed[scope] = true
		}
	}
	return needed["service-lasso:read"] && needed["service-lasso:lifecycle:write"]
}

func hasConnectionScopes(scopes []string,operatorV2 bool)bool{
 if !operatorV2{return hasRequiredScopes(scopes)}
 for _,scope:=range scopes{if scope=="service-lasso:read"{return true}}
 return false
}

func (m *ConnectionManager) Switch(name string) (*Client, error) {
	client, err := m.Client(name)
	if err != nil {
		return nil, err
	}
	m.current = name
	return client, nil
}
