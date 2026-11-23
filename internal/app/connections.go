package app

import "github.com/service-lasso/service-lasso-tui/internal/api"

type connectionManager interface {
	Names() []string
	Current() string
	Switch(string) (*api.Client, error)
}
