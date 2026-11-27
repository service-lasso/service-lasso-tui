package app

import (
	"fmt"
	"math"
	"strings"
	"time"

	"github.com/charmbracelet/lipgloss"
	"github.com/charmbracelet/x/ansi"
	"github.com/service-lasso/service-lasso-tui/internal/api"
)

type stackCheckBucket struct {
	sample  api.OperatorCheckSample
	present bool
	failed  bool
}

func stackCheckBuckets(checks stackServiceChecks, count int) []stackCheckBucket {
	buckets := make([]stackCheckBucket, count)
	start := checks.observed.Add(-api.OperatorCheckWindow)
	for _, sample := range checks.observation.Samples {
		at, _ := time.Parse(time.RFC3339Nano, sample.At)
		position := at.Sub(start)
		if position < 0 || position > api.OperatorCheckWindow {
			continue
		}
		index := min(count-1, int(float64(position)*float64(count)/float64(api.OperatorCheckWindow)))
		buckets[index] = stackCheckBucket{sample, true, buckets[index].failed || !sample.Passed}
	}
	return buckets
}

func stackCheckStrip(checks stackServiceChecks, status string, style lipgloss.Style, width int) string {
	if status != "" {
		glyph := "─"
		if status == "No HTTP check" {
			glyph = "━"
		}
		return operatorRender(style.Foreground(operatorMuted), strings.Repeat(glyph, max(0, width)))
	}
	var line strings.Builder
	for _, bucket := range stackCheckBuckets(checks, width) {
		glyph, color := "·", operatorMuted
		if bucket.present {
			glyph, color = "█", operatorGreen
			if bucket.failed {
				glyph, color = "▂", operatorCoral
			}
		}
		line.WriteString(operatorRender(style.Foreground(color), glyph))
	}
	return line.String()
}

func stackChartMax(buckets []stackCheckBucket) float64 {
	largest := 1.0
	for _, bucket := range buckets {
		if bucket.present {
			largest = max(largest, float64(bucket.sample.DurationMS))
		}
	}
	magnitude := math.Pow10(int(math.Floor(math.Log10(largest / 2))))
	step := largest / (2 * magnitude)
	switch {
	case step > 5:
		return 20 * magnitude
	case step > 2:
		return 10 * magnitude
	case step > 1:
		return 4 * magnitude
	default:
		return 2 * magnitude
	}
}

func stackChartTick(value float64) string {
	if value >= 100000 {
		return fmt.Sprintf("%.0e", value)
	}
	return fmt.Sprintf("%.0f", value)
}

func stackCheckLatencyRows(checks stackServiceChecks, status string, service api.OperatorService, runtimeMetrics string, width, height int, paused bool) []string {
	base := operatorBaseStyle.Foreground(operatorMuted)
	band := operatorBaseStyle.Bold(true).Foreground(operatorTextColor)
	row := func(style lipgloss.Style, text string) string {
		return operatorRender(style, padLine(" "+text, width))
	}
	title := "HTTP check latency · last 15m"
	endpoint := "now"
	if paused {
		endpoint = "read"
		title = "HTTP check latency · paused 15m"
	}
	identity := row(base, "ID: "+operatorText(service.ServiceID))
	latestColor := operatorMuted
	recent := status
	if status == "" {
		latestColor = operatorTeal
		latest := checks.latest()
		latestState := "pass"
		if !latest.Passed {
			latestColor, latestState = operatorAmber, "fail"
		}
		recent = fmt.Sprintf("Latest %d ms · %s", latest.DurationMS, latestState)
	}
	if height < 7 {
		// Reserve graphical space even when the panel cannot fit full axes.
		guideWidth := max(1, width-ansi.StringWidth(recent)-3)
		return []string{identity, row(base, stackCheckStrip(checks, status, base, guideWidth)+" "+recent), row(band, "Runtime: "+stackRunningLabel(service.Running)+" · "+runtimeMetrics)}
	}
	rowCount := min(6, height-6)
	if status != "" && height > 7 {
		rowCount = min(6, height-7)
	}
	preamble := operatorRender(base, padLine(" ID: "+operatorText(service.ServiceID), width-ansi.StringWidth(recent)-2)) +
		operatorRender(band.Foreground(latestColor), " "+recent+" ")
	out := []string{row(band, title), preamble}
	const axisWidth = 7
	columnCount := min(60, width-axisWidth-2)
	buckets := stackCheckBuckets(checks, columnCount)
	if status != "" {
		buckets = make([]stackCheckBucket, columnCount)
	}
	ceiling := stackChartMax(buckets)
	blocks := [...]string{" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"}
	middle := rowCount / 2
	for y := 0; y < rowCount; y++ {
		label := ""
		switch y {
		case 0:
			label = stackChartTick(ceiling)
		case rowCount - 1:
			label = "0"
		case middle:
			label = stackChartTick(ceiling * float64(rowCount-middle) / float64(rowCount))
		}
		if status != "" {
			label = "" // Neutral guides are not measured zero-latency samples.
		}
		line := operatorRender(base, padLine(" "+label, axisWidth))
		for _, bucket := range buckets {
			glyph, color := " ", operatorMuted
			if label != "" || status != "" && y == rowCount-1 {
				glyph, color = "─", operatorBand
			}
			if bucket.present {
				units := max(1, int(math.Ceil(float64(bucket.sample.DurationMS)/ceiling*float64(8*rowCount))))
				level := min(8, units-8*(rowCount-1-y))
				if level > 0 {
					glyph, color = blocks[level], operatorTeal
					if !bucket.sample.Passed {
						glyph, color = "▒", operatorAmber
					}
				}
			}
			line += operatorRender(base.Foreground(color), glyph)
		}
		out = append(out, line+operatorRender(base, strings.Repeat(" ", width-axisWidth-columnCount)))
	}
	axis := strings.Repeat(" ", axisWidth) + "−15m" + strings.Repeat(" ", columnCount-5-len(endpoint)) + endpoint + " "
	out = append(out,
		operatorRender(base, padLine(axis, width)),
		row(base, stackCheckLegend(status)),
		row(band, runtimeMetrics),
	)
	if status != "" && len(out) < height {
		out = append(out, row(base, operatorText(service.Description)))
	}
	if len(out) < height {
		for i := len(checks.observation.Samples) - 1; i >= 0; i-- {
			sample := checks.observation.Samples[i]
			if !sample.Passed {
				at, _ := time.Parse(time.RFC3339Nano, sample.At)
				out = append(out, row(base.Foreground(operatorAmber), fmt.Sprintf("Last failed: %sZ · %d ms", at.UTC().Format("15:04:05"), sample.DurationMS)))
				break
			}
		}
	}
	return out
}

func stackCheckLegend(status string) string {
	if status != "" {
		return "─ No measurement · " + status
	}
	return "█ Pass  ▒ Fail  · Gap"
}
