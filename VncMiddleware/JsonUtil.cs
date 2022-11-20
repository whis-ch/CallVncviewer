using System.Text.Encodings.Web;
using System.Text.Json;

namespace VncMiddleware
{
    internal class JsonUtil
    {
        /// <summary>
        /// Object serialize to json string
        /// </summary>
        /// <typeparam name="T">entity object type</typeparam>
        /// <param name="jsonObject">need eneity object</param>
        /// <returns>string result</returns>
        public static string ObjectToJson<T>(T jsonObject)
        {
            return JsonSerializer.Serialize<T>(jsonObject, new JsonSerializerOptions()
            {
                Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping
            });
        }

        /// <summary>
        /// json string deserialize to object
        /// </summary>
        /// <typeparam name="T">entity object type</typeparam>
        /// <param name="json">object json string</param>
        /// <returns>T</returns>
        public static T JsonToObject<T>(string json)
        {
            return JsonSerializer.Deserialize<T>(json);
        }
    }
}
