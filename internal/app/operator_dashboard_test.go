package app

import (
	"strings"
	"testing"

	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func dashboardTestModel() operatorModel {
	binding := selectedOperatorTestContext("a")
	redis := "service-definition-1"
	generation := "generation-1"
	return operatorModel{
		epoch: 1, width: 120, height: 30, focus: 1, journey: "overview", binding: binding,
		discovery: &api.OperatorDiscovery{},
		services: operatorPageState[api.OperatorService]{Rows: []api.OperatorService{
			operatorJourneyService("demo-api"), operatorJourneyService("demo-web"),
		}, Snapshot: "snapshot", Revision: "revision"},
		service:   "demo-api",
		detail:    &api.OperatorDetail{Summary: operatorJourneyService("demo-api"), Provider: api.OperatorProvider{Role: "service", Availability: api.OperatorAvailability{Supported: true, Available: true}}},
		lifecycle: &api.OperatorLifecycleAvailability{ServiceID: "demo-api", DefinitionRevision: &redis, Generation: &generation, ModelEpoch: &generation, Availability: api.OperatorAvailability{Supported: true, Available: true}, Capabilities: []api.OperatorCapability{{ID: "service_stop", Availability: api.OperatorAvailability{Supported: true, Available: true}}}, MutationBindings: []api.OperatorMutationBinding{{ActionID: "service_stop", ExpectedRevision: &redis, Availability: api.OperatorAvailability{Supported: true, Available: true}}}},
	}
}

func TestDashboardShowsRealServicesAndSummary(t *testing.T) {
	m := dashboardTestModel()
	m.services.Rows[0].Running = "yes"
	m.services.Rows[1].Running = "no"
	view := m.dashboardView()
	for _, want := range []string{"SERVICE ADMIN", "Dashboard", "1/2 running", "1 stopped", "@demo-api", "@demo-web", "←/→ views"} {
		if !strings.Contains(view, want) {
			t.Fatalf("dashboard missing %q\n%s", want, view)
		}
	}
	if strings.Count(view, "▌ ") != 1 {
		t.Fatalf("expected one selected sidebar item\n%s", view)
	}
}

func TestDashboardDetailTabs(t *testing.T) {
	m := dashboardTestModel()
	m.journey = "detail"
	cases := []struct {
		tab  int
		want string
	}{{0, "Provider: service"}, {1, "No dependencies reported."}, {2, "No endpoints reported."}}
	for _, c := range cases {
		m.detailTab = c.tab
		view := m.dashboardView()
		if !strings.Contains(view, c.want) {
			t.Fatalf("tab %d missing %q\n%s", c.tab, c.want, view)
		}
	}
}

func TestDashboardLifecycleRenders(t *testing.T) {
	m := dashboardTestModel()
	m.journey = "lifecycle"
	if view := m.dashboardView(); !strings.Contains(view, "LIFECYCLE") {
		t.Fatalf("lifecycle dashboard missing\n%s", view)
	}
}

func TestDashboardAllJourneysRender(t *testing.T) {
	for _, journey := range operatorViews {
		m := dashboardTestModel()
		m.journey = journey
		if view := m.dashboardView(); view == "" {
			t.Fatalf("journey %s rendered empty", journey)
		}
	}
}
