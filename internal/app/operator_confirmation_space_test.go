package app

import (
	"testing"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

func confirmationSpaceModel(t *testing.T) operatorModel {
	t.Helper()
	binding := selectedOperatorTestContext("a")
	phrase, id, expiry := "confirm service-install 0123456789abcdef0123456789abcdef", "confirmation", "2099-10-08T00:00:00.000Z"
	preview := api.OperatorPreview{PlanID: "plan", ServiceID: "root", ActionID: "service_install", DefinitionRevision: "definition", ExpectedRevision: "revision", Generation: "generation", ModelEpoch: "epoch", Targets: []string{"root"}, Effects: []string{}, Availability: api.OperatorAvailability{Supported: true, Available: true}, Confirmation: api.OperatorConfirmation{Required: true, ID: &id, PhraseKey: &phrase, ExpiresAt: &expiry}, ExpiresAt: expiry}
	if !api.OperatorConfirmationResponseValid(preview, phrase) {
		t.Fatal("invalid confirmation fixture")
	}
	m := operatorModel{epoch: 1, binding: binding, service: "root", journey: "lifecycle", draft: operatorDraft{Dirty: true, Action: "service_install", ReadConnection: 1, ReadService: "root", ReadJourney: "lifecycle", Service: "root", Context: binding, Preview: &preview}}
	m.acceptPageSelection(m.ticket())
	return m
}
func TestOperatorConfirmationPreservesTerminalSpacesAndExactSubmit(t *testing.T) {
	for _, keepSpaces := range []bool{false, true} {
		t.Run(map[bool]string{false: "missing spaces", true: "literal terminal spaces"}[keepSpaces], func(t *testing.T) {
			m := confirmationSpaceModel(t)
			phrase := *m.draft.Preview.Confirmation.PhraseKey
			for _, r := range phrase {
				key := tea.KeyMsg{Type: tea.KeyRunes, Runes: []rune{r}}
				if r == ' ' {
					if !keepSpaces {
						continue
					}
					key = tea.KeyMsg{Type: tea.KeySpace}
				}
				updated, command := m.Update(key)
				m = updated.(operatorModel)
				if command != nil || m.draft.Sent {
					t.Fatal("typing dispatched confirmation")
				}
			}
			if keepSpaces && m.draft.Text != phrase {
				t.Fatalf("terminal phrase changed: %q", m.draft.Text)
			}
			updated, command := m.Update(tea.KeyMsg{Type: tea.KeyEnter})
			m = updated.(operatorModel)
			if keepSpaces {
				if command == nil || !m.draft.Sent {
					t.Fatal("exact phrase not admitted at explicit Enter")
				}
			} else if command != nil || m.draft.Sent || m.errorText != "Enter the exact displayed confirmation phrase." {
				t.Fatal("altered phrase admitted")
			}
		})
	}
}
func TestOperatorConfirmationModifiedSpaceDoesNotAlterPhrase(t *testing.T) {
	m := confirmationSpaceModel(t)
	updated, command := m.Update(tea.KeyMsg{Type: tea.KeySpace, Alt: true})
	m = updated.(operatorModel)
	if command != nil || m.draft.Text != "" || m.draft.Sent {
		t.Fatal("modified space changed confirmation")
	}
}
