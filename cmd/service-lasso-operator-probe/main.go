package main

import (
	"context"
	"encoding/json"
	"fmt"
	"os"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

// Test/local probe for the operator/v2 client against a running Core.
func main() {
	if len(os.Args) < 2 || len(os.Args) > 3 {
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
	binding, err := client.OperatorContext(ctx)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(5)
	}
	fmt.Printf("%s %d %t\n", discovery.MetadataRevision, len(services.Rows), binding.Valid())
	if len(os.Args) == 3 {
		serviceID := os.Args[2]
		detail, err := client.OperatorDetail(ctx, serviceID)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(6)
		}
		availability, err := client.OperatorLifecycleAvailability(ctx, serviceID)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(7)
		}
		revision := ""
		if availability.DefinitionRevision != nil {
			revision = *availability.DefinitionRevision
		}
		preview, err := client.OperatorPreview(ctx, api.OperatorPreviewRequest{
			ServiceID: serviceID, ActionID: "service_start",
			DefinitionRevision: revision, ExpectedRevision: revision,
			Input: json.RawMessage(`{}`),
		})
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(8)
		}
		fmt.Printf("detail %s %s\n", detail.Summary.ServiceID, detail.Summary.DefinitionRevision)
		fmt.Printf("availability %s %t\n", availability.ServiceID, availability.Availability.Available)
		fmt.Printf("preview %s %s\n", preview.ServiceID, preview.PlanID)
	}

}
