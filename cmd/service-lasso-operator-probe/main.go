package main

import (
	"context"
	"fmt"
	"os"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

// Test/local probe for the operator/v2 client against a running Core.
func main() {
	if len(os.Args) != 2 {
		os.Exit(2)
	}
	client, err := api.NewOperatorClientWithAuth(os.Args[1], nil, os.Getenv("SERVICE_LASSO_CORE_TOKEN"), api.AuthModeLocalAdmin)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(2)
	}
	ctx := context.Background()
	discovery, err := client.OperatorDiscovery(ctx)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(3)
	}
	services, err := client.OperatorServices(ctx, nil, "all")
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(4)
	}
	fmt.Printf("%s %d\n", discovery.MetadataRevision, len(services.Rows))
}
