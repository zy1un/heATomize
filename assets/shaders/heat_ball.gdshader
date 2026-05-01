shader_type canvas_item;

uniform vec4 fill_color : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform vec4 outline_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);
uniform float edge_softness : hint_range(0.001, 0.1) = 0.02;
uniform float rim_power : hint_range(0.5, 4.0) = 1.6;

void fragment() {
	vec2 centered_uv = UV - vec2(0.5);
	float distance_to_center = length(centered_uv);

	float outer_radius = 0.5;
	float outer_alpha = 1.0 - smoothstep(outer_radius - edge_softness, outer_radius, distance_to_center);
	float normalized_distance = clamp(distance_to_center / outer_radius, 0.0, 1.0);
	float rim_factor = pow(normalized_distance, rim_power);
	vec4 final_color = mix(fill_color, outline_color, rim_factor);
	final_color.a *= outer_alpha;

	COLOR = final_color;
}
