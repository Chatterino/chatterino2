boost::json::result_for<Status, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Status>, const boost::json::value &jvRoot);

boost::json::result_for<Type, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Type>, const boost::json::value &jvRoot);

boost::json::result_for<BanEvasionEvaluation, boost::json::value>::type
    tag_invoke(boost::json::try_value_to_tag<BanEvasionEvaluation>,
               const boost::json::value &jvRoot);
