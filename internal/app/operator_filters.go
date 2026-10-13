package app

func operatorFilterValues(journey string)[]string{
 switch journey{
 case "overview":return []string{"all","installed","running","unavailable"}
 case "variables":return []string{"all","manifest","global","runtime","derived"}
 case "health_history":return []string{"all","healthy","unhealthy"}
 case "history":return []string{"lifecycle","setup","action","update"}
 case "inbox":return []string{"all","unread","updates","system","workflow","service","errors","hidden"}
 case "queue":return []string{"all","open","acknowledged","deferred"}
 case "audit":return []string{"all","success","failure","denied","skipped"}
 case "logs":return []string{"combined","stdout","stderr","declared"}
 };return nil
}
func(m operatorModel)filter()string{if value:=m.filters[m.journey];value!=""{return value};values:=operatorFilterValues(m.journey);if len(values)>0{return values[0]};return ""}
func(m *operatorModel)cycleFilter()bool{values:=operatorFilterValues(m.journey);if len(values)<2{return false};current:=m.filter();for i,value:=range values{if value==current{if m.filters==nil{m.filters=map[string]string{}};m.filters[m.journey]=values[(i+1)%len(values)];m.invalidate();return true}};return false}
