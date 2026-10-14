package tea

// SourceTerminal binds the actual independently held public terminal operations
// at their original conditional Run callsites. Calls are observations, never
// native authentication, source admission or original disposition authority.
type SourceTerminal interface {
 BeforeFallback() error
 MakePublicRaw() error
 RestorePublicInput() error
 PublicSize()(int,int,error)
}
func WithSourceTerminal(owner SourceTerminal)ProgramOption{return func(p *Program){p.sourceTerminal=owner}}